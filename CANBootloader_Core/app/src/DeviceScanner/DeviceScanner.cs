using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CANBootloaderConsole.DeviceScanner
{
    public class DeviceScanner
    {
        private const int INITIAL_SCAN_WAIT_MS = 2000;
        private const int FIRMWARE_RESPONSE_WAIT_MS = 3000;
        private const int DISCONNECT_SETTLE_DELAY_MS = 100;
        private const int FIRMWARE_REQUEST_DELAY_MS = 100;
        private static readonly (byte Block, byte Id)[] FirmwareVersionRequests =
        {
            (0, 223),
            (9, 0),
            (9, 1),
            (9, 2)
        };

        private CANPort comPort;
        private List<CANDevice> devices;
        private readonly object devicesLock = new object();
        private const byte PCID = 0xFE;
        
        // Public property to control debug logging (can be set from outside)
        public bool EnableDebugLogging { get; set; } = false;

        public async Task<List<CANDevice>> ScanDevicesAsync(string portName)
        {
            devices = new List<CANDevice>();
            comPort = new CANPort();

            ConsoleHelper.WriteInfo($"Opening port {portName}...");

            if (!comPort.Connect(portName))
            {
                ConsoleHelper.WriteError("Failed to connect to COM port.");
                return devices;
            }

            ConsoleHelper.WriteSuccess("Connected successfully");

            // Subscribe to CAN messages
            comPort.CANPort_MSGReceived += OnCANMessageReceived;

            if (!EnableDebugLogging)
            {
                ConsoleHelper.WriteInfo("Scanning CAN bus for devices...");
            }
            else
            {
                ConsoleHelper.WriteInfo("Scanning CAN bus for devices...");
                Console.WriteLine();
            }

            // Send broadcast request for device info
            BroadcastInfoRequest();

            // Wait for initial responses
            await Task.Delay(INITIAL_SCAN_WAIT_MS);

            // Request firmware versions for each discovered device
            await RequestFirmwareVersions();

            // Wait for firmware responses (longer for multiple devices)
            await Task.Delay(FIRMWARE_RESPONSE_WAIT_MS);

            // Unsubscribe BEFORE disconnecting to avoid race condition
            comPort.CANPort_MSGReceived -= OnCANMessageReceived;
            
            // Small delay to let any pending events complete
            await Task.Delay(DISCONNECT_SETTLE_DELAY_MS);
            
            // Now safe to disconnect
            comPort.Disconnect();

            ConsoleHelper.WriteSuccess($"Scan complete. Found {devices.Count} device(s)");

            return devices;
        }

        private void BroadcastInfoRequest()
        {
            byte[] id = { 0x18, 0xEA, 0xFF, PCID };

            if (EnableDebugLogging)
            {
                ConsoleHelper.WriteInfo("=== Broadcasting Info Requests ===");
                ConsoleHelper.WriteInfo("TX: Request Serial Number (PGN 0x01FF00)");
                ConsoleHelper.WriteInfo($"    ID:  {BitConverter.ToString(id)}");
                byte[] msgSerial = { 0x01, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
                ConsoleHelper.WriteInfo($"    MSG: {BitConverter.ToString(msgSerial)}");
                comPort.WriteCAN(id, msgSerial);
                Console.WriteLine();
                ConsoleHelper.WriteInfo("Waiting for responses...");
                Console.WriteLine();
            }
            else
            {
                byte[] msgSerial = { 0x01, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
                comPort.WriteCAN(id, msgSerial);
            }
        }

        private async Task RequestFirmwareVersions()
        {
            // Get the list of devices to request firmware from (outside the lock)
            List<CANDevice> devicesToQuery;

            lock (devicesLock)
            {
                devicesToQuery = devices.ToList();
            }

            // Request firmware versions for each discovered device
            foreach (var device in devicesToQuery)
            {
                if (EnableDebugLogging)
                {
                    ConsoleHelper.WriteInfo($"Requesting firmware versions for CAN ID {device.CanId}...");
                }

                foreach (var (block, id) in FirmwareVersionRequests)
                {
                    SendFirmwareVersionRequest(device.CanId, block, id);
                    await Task.Delay(FIRMWARE_REQUEST_DELAY_MS);
                }
            }
        }

        private void SendFirmwareVersionRequest(byte canId, byte block, byte id)
        {
            comPort.WriteCAN(
                new byte[] { 0x19, 0xEF, canId, PCID },
                new byte[] { 0x00, 0x40, block, id, 0x00, 0x00, 0x00, 0x00 }
            );
        }

        private void OnCANMessageReceived(object sender, CANPort.CANEventArgs e)
        {
            // DEBUG: Log ALL received messages only if debug is enabled
            if (EnableDebugLogging)
            {
                Console.ForegroundColor = ConsoleColor.DarkGray;
                Console.WriteLine($"RX: ID_MSG=0x{e.ID_MSG:X6} (0x{e.ID_MSG:X8}), ID_Unit={e.ID_Unit}, MSG=[{BitConverter.ToString(e.MSG)}]");
                Console.ResetColor();
            }

            // Handle different message types
            switch (e.ID_MSG)
            {
                case 0x19FF00: // Serial number message
                case 0x18FF07:
                    if (EnableDebugLogging)
                    {
                        Console.ForegroundColor = ConsoleColor.Yellow;
                        Console.WriteLine($"    --> Handling Serial Number");
                        Console.ResetColor();
                    }
                    HandleSerialNumber(e);
                    break;

                case 0x19FF03: // Part number high
                    if (EnableDebugLogging)
                    {
                        Console.ForegroundColor = ConsoleColor.Yellow;
                        Console.WriteLine($"    --> Handling Part Number High");
                        Console.ResetColor();
                    }
                    HandlePartNumberHigh(e);
                    break;

                case 0x19FF04: // Part number low
                    if (EnableDebugLogging)
                    {
                        Console.ForegroundColor = ConsoleColor.Yellow;
                        Console.WriteLine($"    --> Handling Part Number Low");
                        Console.ResetColor();
                    }
                    HandlePartNumberLow(e);
                    break;
            }

            // Decode Block/ID for firmware versions (matches GUI logic)
            if ((e.ID_MSG >> 8) == 0x19EF)
            {
                if (EnableDebugLogging)
                {
                    Console.ForegroundColor = ConsoleColor.Magenta;
                    Console.WriteLine($"    --> Firmware Response Detected (Block/ID message)");
                    Console.ResetColor();
                }

                // App Settings Request Value
                if (e.MSG[1] == 0x40)
                {
                    if (EnableDebugLogging)
                    {
                        Console.ForegroundColor = ConsoleColor.Yellow;
                        Console.WriteLine($"    --> Handling Firmware Version (Block={e.MSG[2]}, ID={e.MSG[3]})");
                        Console.ResetColor();
                    }
                    HandleFirmwareResponse(e);
                }
            }
        }

        private void HandleSerialNumber(CANPort.CANEventArgs e)
        {
            lock (devicesLock)
            {
                var device = GetOrCreateDevice(e.ID_Unit);

                // Calculate serial number: Major (6 digits) and Minor (4 digits)
                ulong serialMajor = (ulong)e.MSG[3] * 10000 + (ulong)e.MSG[4] * 100 + (ulong)e.MSG[5];
                ulong serialMinor = (ulong)e.MSG[6] * 100 + (ulong)e.MSG[7];

                device.SerialNumber = serialMajor * 10000 + serialMinor;

                // Format as "XXXXXX-XXXX"
                device.SerialString = $"{serialMajor:D6}-{serialMinor:D4}";
                device.LastSeen = DateTime.Now;

                if (EnableDebugLogging)
                {
                    Console.ForegroundColor = ConsoleColor.Green;
                    Console.WriteLine($"        Serial Number: {device.SerialString}");
                    Console.ResetColor();
                }
            }
        }

        private void HandleFirmwareResponse(CANPort.CANEventArgs e)
        {
            lock (devicesLock)
            {
                var device = GetOrCreateDevice(e.ID_Unit);

                // Switch on Block
                switch (e.MSG[2])
                {
                    case 0: // Control Board
                        {
                            // ID 223 = Software Version - Control Board
                            if (e.MSG[3] == 223)
                            {
                                // Extract the last 4 bytes
                                byte[] lastFourBytes = new byte[] { e.MSG[4], e.MSG[5], e.MSG[6], e.MSG[7] };
                                int value = BitConverter.ToInt32(lastFourBytes, 0);
                                device.BridgeFirmwareVersions[1] = value;

                                if (EnableDebugLogging)
                                {
                                    Console.ForegroundColor = ConsoleColor.Green;
                                    Console.WriteLine($"        Bridge 1 Firmware: {CANDevice.ConvertFirmwareToString(value)}");
                                    Console.ResetColor();
                                }
                            }
                        }
                        break;

                    case 9: // Power Board, Display, DCDC
                        {
                            // Extract the last 4 bytes
                            byte[] lastFourBytes = new byte[] { e.MSG[4], e.MSG[5], e.MSG[6], e.MSG[7] };
                            int value = BitConverter.ToInt32(lastFourBytes, 0);

                            // 9,0 = Power board firmware
                            if (e.MSG[3] == 0)
                            {
                                device.BridgeFirmwareVersions[2] = value;
                                if (EnableDebugLogging)
                                {
                                    Console.ForegroundColor = ConsoleColor.Green;
                                    Console.WriteLine($"        Bridge 2 Firmware: {CANDevice.ConvertFirmwareToString(value)}");
                                    Console.ResetColor();
                                }
                            }
                            // 9,1 = Display firmware
                            else if (e.MSG[3] == 1)
                            {
                                device.BridgeFirmwareVersions[3] = value;
                                if (EnableDebugLogging)
                                {
                                    Console.ForegroundColor = ConsoleColor.Green;
                                    Console.WriteLine($"        Bridge 3 Firmware: {CANDevice.ConvertFirmwareToString(value)}");
                                    Console.ResetColor();
                                }
                            }
                            // 9,2 = DCDC firmware
                            else if (e.MSG[3] == 2)
                            {
                                device.BridgeFirmwareVersions[4] = value;
                                if (EnableDebugLogging)
                                {
                                    Console.ForegroundColor = ConsoleColor.Green;
                                    Console.WriteLine($"        Bridge 4 Firmware: {CANDevice.ConvertFirmwareToString(value)}");
                                    Console.ResetColor();
                                }
                            }
                        }
                        break;
                }

                device.LastSeen = DateTime.Now;
            }
        }

        private void HandlePartNumberHigh(CANPort.CANEventArgs e)
        {
            lock (devicesLock)
            {
                var device = GetOrCreateDevice(e.ID_Unit);
                bool isEmpty = e.MSG.All(b => b == 0xFF);
                if (!isEmpty)
                {
                    int length = Array.IndexOf(e.MSG, (byte)0);
                    if (length < 0) length = e.MSG.Length;
                    device.PartNumber = Encoding.UTF8.GetString(e.MSG, 0, length);

                    if (EnableDebugLogging)
                    {
                        Console.ForegroundColor = ConsoleColor.Green;
                        Console.WriteLine($"        Part Number (High): '{device.PartNumber}' (Length: {length})");
                        Console.ResetColor();
                    }
                }
                else if (EnableDebugLogging)
                {
                    Console.ForegroundColor = ConsoleColor.DarkYellow;
                    Console.WriteLine($"        Part Number (High): EMPTY (all 0xFF)");
                    Console.ResetColor();
                }
                device.LastSeen = DateTime.Now;
            }
        }

        private void HandlePartNumberLow(CANPort.CANEventArgs e)
        {
            lock (devicesLock)
            {
                var device = GetOrCreateDevice(e.ID_Unit);
                bool isEmpty = e.MSG.All(b => b == 0xFF);
                if (!isEmpty)
                {
                    int length = Array.IndexOf(e.MSG, (byte)0);
                    if (length < 0) length = e.MSG.Length;
                    string lowPart = Encoding.UTF8.GetString(e.MSG, 0, length);
                    device.PartNumber += lowPart;

                    if (EnableDebugLogging)
                    {
                        Console.ForegroundColor = ConsoleColor.Green;
                        Console.WriteLine($"        Part Number (Low): '{lowPart}' (Length: {length})");
                        Console.WriteLine($"        Complete Part Number: '{device.PartNumber}'");
                        Console.ResetColor();
                    }
                }
                else if (EnableDebugLogging)
                {
                    Console.ForegroundColor = ConsoleColor.DarkYellow;
                    Console.WriteLine($"        Part Number (Low): EMPTY (all 0xFF)");
                    Console.ResetColor();
                }
                device.LastSeen = DateTime.Now;
            }
        }

        private CANDevice GetOrCreateDevice(byte canId)
        {
            var device = devices.FirstOrDefault(d => d.CanId == canId);
            if (device == null)
            {
                device = new CANDevice(canId);
                devices.Add(device);

                if (EnableDebugLogging)
                {
                    Console.ForegroundColor = ConsoleColor.Cyan;
                    Console.WriteLine($"    --> NEW DEVICE CREATED: CAN ID {canId}");
                    Console.ResetColor();
                }
            }
            return device;
        }

        public static void DisplayDevices(List<CANDevice> devices)
        {
            Console.WriteLine();
            ConsoleHelper.WriteHeader($"Discovered Devices ({devices.Count})");

            if (devices.Count == 0)
            {
                ConsoleHelper.WriteWarning("No devices found on CAN bus.");
                return;
            }

            foreach (var device in devices.OrderBy(d => d.CanId))
            {
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine($"  CAN ID: {device.CanId}");
                Console.ForegroundColor = ConsoleColor.Gray;
                Console.WriteLine($"    Part Number: {(string.IsNullOrEmpty(device.PartNumber) ? "Unknown" : device.PartNumber)}");
                Console.WriteLine($"    Serial:      {(string.IsNullOrEmpty(device.SerialString) ? "Unknown" : device.SerialString)}");

                // Display firmware versions for all bridges
                if (device.BridgeFirmwareVersions.Count > 0)
                {
                    Console.WriteLine($"    Firmware Versions:");
                    foreach (var bridge in device.BridgeFirmwareVersions.OrderBy(b => b.Key))
                    {
                        string bridgeName = GetBridgeName(bridge.Key);
                        string firmwareVersion = CANDevice.ConvertFirmwareToString(bridge.Value);

                        // Only show non-zero firmware versions
                        if (bridge.Value > 0)
                        {
                            Console.WriteLine($"      Bridge {bridge.Key} ({bridgeName}): v{firmwareVersion}");
                        }
                    }
                }
                else
                {
                    Console.WriteLine($"    Firmware:    No firmware versions available");
                }

                Console.ResetColor();
                Console.WriteLine();
            }
        }

        private static string GetBridgeName(byte bridgeId)
        {
            return bridgeId switch
            {
                1 => "Control Board",
                2 => "Power Board",
                3 => "Display",
                4 => "DCDC",
                _ => "Unknown"
            };
        }
    }
}