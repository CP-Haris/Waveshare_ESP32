using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Timers;

namespace CANBootloaderConsole
{
    public class CANPort : SerialPort
    {
        private const int RxTimeoutMs = 200;
        private const int FullFrameLength = 30;
        private const int PayloadWithPrefixLength = 25;
        private const int CrcHexLength = 4;
        private const int CanIdByteLength = 4;
        private const int CanDataByteLength = 8;

        private readonly Timer TMR_COM_TIMEOUT;
        private readonly StringBuilder builder_CAN_RX = new StringBuilder();
        private bool CAN_Receiving = false;
        private readonly object rxLock = new object();
        private bool handlerAttached = false;

        public CANPort()
        {
            TMR_COM_TIMEOUT = new Timer(100);
            TMR_COM_TIMEOUT.Elapsed += OnTimedEvent;
            TMR_COM_TIMEOUT.AutoReset = true;
            TMR_COM_TIMEOUT.Enabled = false;
        }

        private void OnTimedEvent(object source, ElapsedEventArgs e)
        {
            lock (rxLock)
            {
                TMR_COM_TIMEOUT.Stop();
                builder_CAN_RX.Clear();
                CAN_Receiving = false;
            }
        }

        public bool Connect(string COM_Port)
        {
            if (!this.IsOpen)
            {
                this.PortName = COM_Port;
                this.BaudRate = 115200;
                this.Parity = Parity.None;
                this.DataBits = 8;
                this.StopBits = StopBits.One;
                this.Handshake = OperatingSystem.IsWindows() ? Handshake.RequestToSend : Handshake.None;
                this.ReadTimeout = OperatingSystem.IsWindows() ? 100 : 3000;
                this.WriteTimeout = OperatingSystem.IsWindows() ? 200 : 3000; // Reduced from 500ms
                this.WriteBufferSize = 500000;
                this.ReadBufferSize = 500000;
                
                // macOS specific settings for better stability
                if (!OperatingSystem.IsWindows())
                {
                    this.DtrEnable = false;
                    this.RtsEnable = false;
                }

                try
                {
                    this.Open();
                    
                    // Clear buffers and stabilize connection on macOS
                    if (!OperatingSystem.IsWindows())
                    {
                        this.DiscardInBuffer();
                        this.DiscardOutBuffer();
                        System.Threading.Thread.Sleep(100);
                    }
                    
                    if (!handlerAttached)
                    {
                        this.DataReceived += EVENT_RX;
                        handlerAttached = true;
                    }
                }
                catch (UnauthorizedAccessException)
                {
                    ConsoleHelper.WriteError($"Access denied to port {COM_Port}. Port may be in use by another application.");
                    return false;
                }
                catch (IOException ex)
                {
                    ConsoleHelper.WriteError($"Port {COM_Port} not found or unavailable: {ex.Message}");
                    return false;
                }
                catch (Exception ex)
                {
                    ConsoleHelper.WriteError($"Failed to open port {COM_Port}: {ex.Message}");
                    return false;
                }
            }
            return this.IsOpen;
        }

        public bool Disconnect()
        {
            if (this.IsOpen)
            {
                if (handlerAttached)
                {
                    this.DataReceived -= EVENT_RX;
                    handlerAttached = false;
                }
                this.Close();
            }
            return this.IsOpen;
        }

        public event EventHandler<CANEventArgs> CANPort_MSGReceived;

        public class CANEventArgs : EventArgs
        {
            public byte ID_Unit { get; set; }
            public uint ID_MSG { get; set; }
            public byte[] MSG { get; set; }
        }

        private void EVENT_RX(object sender, SerialDataReceivedEventArgs e)
        {
            try
            {
                if (!this.IsOpen) return;
                
                int bytesToRead = this.BytesToRead;
                if (bytesToRead <= 0) return;
                
                // Read ALL available bytes at once - this is critical for multi-device support
                byte[] buffer = new byte[bytesToRead];
                int bytesRead = this.Read(buffer, 0, bytesToRead);
                
                // Process each byte
                for (int i = 0; i < bytesRead; i++)
                {
                    char UART_Data = (char)buffer[i];

                    if (UART_Data == 'M')
                    {
                        HandleStartByte();
                    }
                    else if (CAN_Receiving)
                    {
                        HandleDataByte(UART_Data);
                    }
                }
            }
            catch (System.IO.IOException)
            {
                // Port was closed while reading - this is expected during disconnect
            }
            catch (InvalidOperationException)
            {
                // Port is not open - ignore
            }
        }

        private void HandleStartByte()
        {
            if (builder_CAN_RX.Length > 0)
            {
                builder_CAN_RX.Clear();
            }

            CAN_Receiving = true;
            builder_CAN_RX.Append('M');
            ResetTimeoutTimer();
        }

        private void HandleDataByte(char UART_Data)
        {
            builder_CAN_RX.Append(UART_Data);

            if (UART_Data == '\r' && builder_CAN_RX.Length == FullFrameLength)
            {
                TMR_COM_TIMEOUT.Stop();
                CAN_Receiving = false;
                ProcessReceivedMessage();
            }
            else if (builder_CAN_RX.Length > FullFrameLength)
            {
                builder_CAN_RX.Clear();
                CAN_Receiving = false;
                TMR_COM_TIMEOUT.Stop();
            }
        }

        private void ProcessReceivedMessage()
        {
            if (builder_CAN_RX.Length < FullFrameLength)
            {
                builder_CAN_RX.Clear();
                return;
            }

            string message = builder_CAN_RX.ToString(0, PayloadWithPrefixLength);
            string crc = builder_CAN_RX.ToString(PayloadWithPrefixLength, CrcHexLength);

            byte[] messageBytes = Encoding.ASCII.GetBytes(message);
            string calculatedCrc = CalculateCRC(messageBytes).ToString("X4");

            if (string.Equals(calculatedCrc, crc, StringComparison.OrdinalIgnoreCase))
            {
                try
                {
                    ParseAndRaiseEvent(message);
                }
                catch (Exception ex)
                {
                    ConsoleHelper.WriteWarning($"Message parsing failed: {ex.Message}");
                }
            }
            else
            {
                if (OperatorSettings.CurrentMode == OperatorMode.Advanced)
                {
                    ConsoleHelper.WriteDebug($"CRC mismatch - Expected: {calculatedCrc}, Got: {crc}");
                }
                builder_CAN_RX.Clear();
            }
        }

        private void ParseAndRaiseEvent(string message)
        {
            if (message.Length < PayloadWithPrefixLength)
                throw new ArgumentException("Message too short for parsing.");

            byte[] id = Functions.HexStrToByteArray(message.Substring(1, 8));
            byte[] msg = Functions.HexStrToByteArray(message.Substring(9, 16));

            if (id.Length < CanIdByteLength)
                throw new ArgumentException("ID array too short.");
            if (msg.Length < CanDataByteLength)
                throw new ArgumentException("MSG array too short.");

            uint msg_id = (uint)(id[0] << 16) + (uint)(id[1] << 8) + id[2];
            byte unit_id = id[3];

            builder_CAN_RX.Clear();

            // Use ThreadPool to invoke event asynchronously - prevents blocking RX
            var handler = CANPort_MSGReceived;
            if (handler != null)
            {
                var args = new CANEventArgs { ID_Unit = unit_id, ID_MSG = msg_id, MSG = msg };
                System.Threading.ThreadPool.QueueUserWorkItem(_ => handler(this, args));
            }
        }

        private void ResetTimeoutTimer()
        {
            TMR_COM_TIMEOUT.Stop();
            TMR_COM_TIMEOUT.Interval = RxTimeoutMs;
            TMR_COM_TIMEOUT.Start();
        }

        public void WriteCAN(byte[] id, byte[] msg)
        {
            if (id == null || msg == null || id.Length != CanIdByteLength || msg.Length != CanDataByteLength)
            {
                if (OperatorSettings.CurrentMode == OperatorMode.Advanced)
                {
                    ConsoleHelper.WriteWarning("Invalid CAN frame. Expected ID length 4 and MSG length 8.");
                }
                return;
            }

            var message = new StringBuilder("M");
            foreach (byte b in id) message.AppendFormat("{0:X2}", b);
            foreach (byte b in msg) message.AppendFormat("{0:X2}", b);

            if (message.Length == PayloadWithPrefixLength)
            {
                byte[] messageBytes = Encoding.ASCII.GetBytes(message.ToString());
                string crc = CalculateCRC(messageBytes).ToString("X4");
                message.Append(crc).Append("\r");
                try
                {
                    byte[] finalMessage = Encoding.ASCII.GetBytes(message.ToString());
                    this.BaseStream.Write(finalMessage, 0, finalMessage.Length);
                    
                    // Flush to ensure immediate transmission
                    this.BaseStream.Flush();
                }
                catch (InvalidOperationException)
                {
                    ConsoleHelper.WriteError("Port is not open. Cannot send CAN message.");
                }
                catch (TimeoutException)
                {
                    ConsoleHelper.WriteWarning("Write timeout - device may not be responding.");
                }
                catch (Exception ex)
                {
                    ConsoleHelper.WriteError($"Write failed: {ex.Message}");
                }
            }
        }

        private static ushort CalculateCRC(byte[] data)
        {
            ushort wCRC = 0;
            foreach (byte b in data)
            {
                wCRC ^= (ushort)(b << 8);
                for (int i = 0; i < 8; i++)
                    wCRC = (wCRC & 0x8000) != 0 ? (ushort)((wCRC << 1) ^ 0x1021) : (ushort)(wCRC << 1);
            }
            return wCRC;
        }

        public static void ListAvailablePorts()
        {
            ConsoleHelper.WriteHeader("Available COM Ports");

            try
            {
                string[] portNames = SerialPort.GetPortNames();

                if (portNames.Length == 0)
                {
                    ConsoleHelper.WriteWarning("No COM ports detected on this system.");
                    return;
                }

                // Sort port names for cleaner output
                Array.Sort(portNames);

                foreach (string portName in portNames)
                {
                    var portInfo = GetPortHardwareInfo(portName);
                    
                    Console.ForegroundColor = ConsoleColor.Cyan;
                    Console.WriteLine($"\n  Port: {portName}");
                    Console.ResetColor();

                    if (portInfo != null)
                    {
                        if (!string.IsNullOrEmpty(portInfo.Description))
                        {
                            Console.ForegroundColor = ConsoleColor.White;
                            Console.WriteLine($"    Description: {portInfo.Description}");
                        }

                        if (!string.IsNullOrEmpty(portInfo.VID))
                        {
                            Console.ForegroundColor = ConsoleColor.Green;
                            Console.WriteLine($"    VID:         {portInfo.VID}");
                        }

                        if (!string.IsNullOrEmpty(portInfo.PID))
                        {
                            Console.ForegroundColor = ConsoleColor.Green;
                            Console.WriteLine($"    PID:         {portInfo.PID}");
                        }

                        if (!string.IsNullOrEmpty(portInfo.HardwareID))
                        {
                            Console.ForegroundColor = ConsoleColor.Gray;
                            Console.WriteLine($"    Hardware ID: {portInfo.HardwareID}");
                        }

                        if (!string.IsNullOrEmpty(portInfo.Manufacturer))
                        {
                            Console.ForegroundColor = ConsoleColor.Yellow;
                            Console.WriteLine($"    Manufacturer: {portInfo.Manufacturer}");
                        }
                    }
                    else
                    {
                        Console.ForegroundColor = ConsoleColor.DarkGray;
                        Console.WriteLine("    (No device information available)");
                    }
                    Console.ResetColor();
                }
            }
            catch (Exception ex)
            {
                ConsoleHelper.WriteError($"Failed to enumerate ports: {ex.Message}");
            }
        }

        public static string FindPortByVidPid(params string[] vidPidPairs)
        {
            try
            {
                string[] portNames = SerialPort.GetPortNames();

                if (OperatingSystem.IsWindows())
                {
                    // Windows: Use WMI to detect by VID/PID
                    foreach (string portName in portNames)
                    {
                        var portInfo = GetPortHardwareInfo(portName);

                        if (portInfo != null && !string.IsNullOrEmpty(portInfo.VID) && !string.IsNullOrEmpty(portInfo.PID))
                        {
                            // Check if this port matches any of the VID/PID pairs
                            for (int i = 0; i < vidPidPairs.Length; i += 2)
                            {
                                if (i + 1 < vidPidPairs.Length)
                                {
                                    string targetVid = vidPidPairs[i];
                                    string targetPid = vidPidPairs[i + 1];

                                    if (string.Equals(portInfo.VID, targetVid, StringComparison.OrdinalIgnoreCase) &&
                                        string.Equals(portInfo.PID, targetPid, StringComparison.OrdinalIgnoreCase))
                                    {
                                        if (OperatorSettings.CurrentMode == OperatorMode.Advanced)
                                        {
                                            ConsoleHelper.WriteSuccess($"Auto-detected device on {portName}");
                                            ConsoleHelper.WriteInfo($"  VID:PID = {portInfo.VID}:{portInfo.PID}");
                                            if (!string.IsNullOrEmpty(portInfo.Description))
                                            {
                                                ConsoleHelper.WriteInfo($"  Device: {portInfo.Description}");
                                            }
                                        }
                                        return portName;
                                    }
                                }
                            }
                        }
                    }

                    // Build list of searched VID/PID for error message
                    var searchedPairs = new List<string>();
                    for (int i = 0; i < vidPidPairs.Length; i += 2)
                    {
                        if (i + 1 < vidPidPairs.Length)
                        {
                            searchedPairs.Add($"{vidPidPairs[i]}:{vidPidPairs[i + 1]}");
                        }
                    }

                    ConsoleHelper.WriteWarning($"No device found with VID:PID = {string.Join(" or ", searchedPairs)}");
                    return null;
                }
                else
                {
                    // Linux/macOS logic
                    if (portNames.Length > 0)
                    {
                        // Filter out Bluetooth and debug ports
                        var filteredPorts = portNames.Where(p => 
                            !p.Contains("Bluetooth", StringComparison.OrdinalIgnoreCase) &&
                            !p.Contains("debug-console", StringComparison.OrdinalIgnoreCase)
                        ).ToArray();

                        if (filteredPorts.Length == 0)
                        {
                            ConsoleHelper.WriteWarning("Only Bluetooth/debug ports found. Skipping auto-detection.");
                            return null;
                        }

                        // Prefer cu.* over tty.* on macOS (cu.* for callout/output)
                        string firstCuPort = Array.Find(filteredPorts, p => p.StartsWith("/dev/cu."));
                        if (firstCuPort != null)
                        {
                            if (OperatorSettings.CurrentMode == OperatorMode.Advanced)
                            {
                                ConsoleHelper.WriteSuccess($"Using detected port: {firstCuPort}");
                            }
                            return firstCuPort;
                        }

                        // Linux default port
                        string defaultPort = "/dev/ttyUSB0";
                        if (Array.Exists(filteredPorts, p => p == defaultPort))
                        {
                            if (OperatorSettings.CurrentMode == OperatorMode.Advanced)
                            {
                                ConsoleHelper.WriteSuccess($"Using default port: {defaultPort}");
                            }
                            return defaultPort;
                        }

                        // Look for USB or ACM ports (Linux)
                        string firstUsbPort = Array.Find(filteredPorts, p => 
                            p.Contains("USB", StringComparison.OrdinalIgnoreCase) || 
                            p.Contains("ACM", StringComparison.OrdinalIgnoreCase));
                        if (firstUsbPort != null)
                        {
                            if (OperatorSettings.CurrentMode == OperatorMode.Advanced)
                            {
                                ConsoleHelper.WriteSuccess($"Using detected port: {firstUsbPort}");
                            }
                            return firstUsbPort;
                        }

                        // Use first filtered port as fallback
                        ConsoleHelper.WriteWarning($"Using first available port: {filteredPorts[0]}");
                        return filteredPorts[0];
                    }

                    ConsoleHelper.WriteError("No serial ports found on this system.");
                    return null;
                }
            }
            catch (Exception ex)
            {
                ConsoleHelper.WriteError($"Auto-detection failed: {ex.Message}");
                return null;
            }
        }

        public class PortHardwareInfo
        {
            public string PortName { get; set; }
            public string Description { get; set; }
            public string HardwareID { get; set; }
            public string VID { get; set; }
            public string PID { get; set; }
            public string Manufacturer { get; set; }
        }

        private static PortHardwareInfo GetPortHardwareInfo(string portName)
        {
            try
            {
#pragma warning disable CA1416 // Validate platform compatibility
                // Use WMI to get hardware information (Windows only)
                if (OperatingSystem.IsWindows())
                {
                    return GetWindowsPortHardwareInfo(portName);
                }
#pragma warning restore CA1416 // Validate platform compatibility
                else
                {
                    // For Linux/macOS, return basic info
                    return new PortHardwareInfo
                    {
                        PortName = portName,
                        Description = "Unix/Linux device (use 'ls -l /dev/tty*' for details)"
                    };
                }
            }
            catch (Exception ex)
            {
                ConsoleHelper.WriteWarning($"Could not get hardware info for {portName}: {ex.Message}");
                return new PortHardwareInfo
                {
                    PortName = portName,
                    Description = "Unknown device"
                };
            }
        }

        [System.Runtime.Versioning.SupportedOSPlatform("windows")]
        private static PortHardwareInfo GetWindowsPortHardwareInfo(string portName)
        {
#pragma warning disable CA1416 // Validate platform compatibility
            using (var searcher = new System.Management.ManagementObjectSearcher(
                $"SELECT * FROM Win32_PnPEntity WHERE Name LIKE '%({portName})%'"))
            {
                foreach (System.Management.ManagementObject obj in searcher.Get())
                {
                    var portInfo = new PortHardwareInfo
                    {
                        PortName = portName
                    };

                    // Get friendly name/description
                    string name = obj["Name"]?.ToString();
                    if (!string.IsNullOrEmpty(name))
                    {
                        // Remove port name from description
                        int portIndex = name.LastIndexOf($"({portName})");
                        if (portIndex > 0)
                        {
                            portInfo.Description = name.Substring(0, portIndex).Trim();
                        }
                        else
                        {
                            portInfo.Description = name;
                        }
                    }

                    // Get Device ID (Hardware ID)
                    string deviceId = obj["DeviceID"]?.ToString();
                    if (!string.IsNullOrEmpty(deviceId))
                    {
                        portInfo.HardwareID = deviceId;

                        // Extract VID and PID from Device ID
                        // Format is typically: USB\VID_XXXX&PID_XXXX\...
                        var vidMatch = Regex.Match(
                            deviceId, 
                            @"VID_([0-9A-F]{4})", 
                            RegexOptions.IgnoreCase);
                        
                        if (vidMatch.Success)
                        {
                            portInfo.VID = vidMatch.Groups[1].Value;
                        }

                        var pidMatch = Regex.Match(
                            deviceId, 
                            @"PID_([0-9A-F]{4})", 
                            RegexOptions.IgnoreCase);
                        
                        if (pidMatch.Success)
                        {
                            portInfo.PID = pidMatch.Groups[1].Value;
                        }
                    }

                    // Get Manufacturer
                    string manufacturer = obj["Manufacturer"]?.ToString();
                    if (!string.IsNullOrEmpty(manufacturer))
                    {
                        portInfo.Manufacturer = manufacturer;
                    }

                    return portInfo;
                }
            }
#pragma warning restore CA1416 // Validate platform compatibility

            // If no device found in WMI, return basic info
            return new PortHardwareInfo
            {
                PortName = portName,
                Description = "Windows device (no WMI details available)"
            };
        }
        
        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                // Unhook event handler before closing
                if (handlerAttached)
                {
                    this.DataReceived -= EVENT_RX;
                    handlerAttached = false;
                }
                TMR_COM_TIMEOUT?.Dispose();
            }
            base.Dispose(disposing);
        }
    }

    public static class ConsoleHelper
    {
        public static void WriteSuccess(string message)
        {
            Console.ForegroundColor = ConsoleColor.Green;
            Console.WriteLine($"\u2713 {message}");
            Console.ResetColor();
        }

        public static void WriteError(string message)
        {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine($"\u2717 {message}");
            Console.ResetColor();
        }

        public static void WriteWarning(string message)
        {
            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.WriteLine($"\u26A0 {message}");
            Console.ResetColor();
        }

        public static void WriteInfo(string message)
        {
            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.WriteLine($"\u00BB {message}");
            Console.ResetColor();
        }
        
        // New: Info message that respects operator mode
        public static void WriteInfoAdvanced(string message)
        {
            if (OperatorSettings.IsAtLeast(OperatorMode.Advanced))
            {
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine($"\u00BB {message}");
                Console.ResetColor();
            }
        }

        public static void WriteDebug(string message)
        {
            // Debug output disabled for cleaner advanced mode display
        }

        public static void WriteHeader(string message)
        {
            Console.ForegroundColor = ConsoleColor.White;
            Console.WriteLine($"\n{message}");
            Console.WriteLine(new string('-', message.Length));
            Console.ResetColor();
        }

        public static void WriteProgressBar(int percent, int current, int total)
        {
            const int barLength = 50;
            int filled = (percent * barLength) / 100;
            char filledChar = '\u2593'; // Dark shade (looks like CP437 178)
            char emptyChar = '\u2591'; // Light shade (looks like CP437 176)

            // Move to start of line without clearing (reduces flicker)
            Console.Write("\r");
            
            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.Write("[");

            Console.ForegroundColor = ConsoleColor.Green;
            Console.Write(new string(filledChar, filled));

            Console.ForegroundColor = ConsoleColor.DarkGray;
            Console.Write(new string(emptyChar, barLength - filled));

            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.Write("] ");

            Console.ForegroundColor = ConsoleColor.White;
            Console.Write($"{percent,3}% ");

            Console.ForegroundColor = ConsoleColor.Gray;
            Console.Write($"({current}/{total})");
            
            // Pad with spaces to overwrite any leftover characters
            Console.Write(new string(' ', 10));

            Console.ResetColor();
        }
    }
}