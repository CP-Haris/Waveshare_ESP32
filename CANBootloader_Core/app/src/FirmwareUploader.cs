using System;
using System.Threading;
using System.Threading.Tasks;

namespace CANBootloaderConsole
{
    public class FirmwareUploader
    {
        private BootloaderProtocol protocol;
        private CANPort comPort;
        private CancellationTokenSource cancellationTokenSource;
        private bool isComplete = false;
        private bool hasError = false;
        private bool bootloaderReady = false;
        private bool bridgeOpened = false;
        private byte productId = 0;
        private byte currentBridgeId = 0;
        private ulong bootloaderSerial = 0;
        private byte applicationCanId = 0;
        private byte bootloaderCanId = 0;
        private ulong applicationSerial = 0;
        private readonly object bootloaderLock = new object();
        private const ulong NO_SERIAL_VALUE = 25757575755;
        private const int BOOTLOADER_STATUS_MESSAGE_ID = 0x19FF20;
        private const int BOOTLOADER_STATUS_MESSAGE_LENGTH = 8;
        private const byte BOOTLOADER_STATUS_READY = 199;
        private const byte BOOTLOADER_STATUS_ACK = 99;
        private const int BOOTLOADER_ENTRY_DELAY_MS = 1500;
        private const int HEARTBEAT_HANDLER_READY_DELAY_MS = 200;
        private const int HEARTBEAT_WAIT_POLL_MS = 100;
        private const double HEARTBEAT_WAIT_SECONDS_NORMAL = 5.0;
        private const double HEARTBEAT_WAIT_SECONDS_ALREADY_IN_BOOTLOADER = 2.0;
        private const int BRIDGE_OPEN_TIMEOUT_SECONDS = 5;
        private const int BRIDGE_INIT_DELAY_MS = 5000;
        private const int BOOTLOADER_SERIAL_COPY_DELAY_MS = 500;
        private const int EXIT_BOOTLOADER_DELAY_MS = 500;
        private const int EARLY_EXIT_BOOTLOADER_DELAY_MS = 300;
        private static readonly byte[] NO_SERIAL_BYTES = { 25, 75, 75, 75, 55 };
        private bool otherBootloaderDetected = false;
        private byte otherBootloaderCanId = 0;
        private ulong otherBootloaderSerial = 0;
        private int totalBootloadersDetected = 0;
        private const int BOOTLOADER_SCAN_TIMEOUT_MS = 600;
        private const int BOOTLOADER_SCAN_POLL_MS = 50;
        private const int PROGRESS_POLL_MS = 100;
        
        // Logging support
        private IOperationLogger logger;
        private string logUserIdentification;
        private string logOriginalVersion;
        private string logTargetVersion;
        private string logApplicationSn;
        private string logBootloaderSn;
        private string logBridgeId;
        private string logPartNumber;

        private static bool IsAdvancedMode() => OperatorSettings.IsAtLeast(OperatorMode.Advanced);

        private static bool IsDebugMode() => OperatorSettings.CurrentMode == OperatorMode.Advanced;

        private static bool IsSimpleMode() => OperatorSettings.CurrentMode == OperatorMode.Simple;

        private static string FormatSerial(ulong serial)
        {
            return serial == NO_SERIAL_VALUE ? "NO SERIAL" : serial.ToString().PadLeft(10, '0');
        }

        private void CleanupConnection()
        {
            if (comPort == null)
            {
                return;
            }

            comPort.CANPort_MSGReceived -= OnBootloaderStatusReceived;
            comPort.Disconnect();
        }

        private void ResetOperationState()
        {
            // Reset per-operation state to keep behavior predictable if this instance is reused.
            isComplete = false;
            hasError = false;
            bootloaderReady = false;
            bridgeOpened = false;
            productId = 0;
            currentBridgeId = 0;
            bootloaderSerial = 0;
            bootloaderCanId = 0;

            // Reset bootloader detection counters
            otherBootloaderDetected = false;
            otherBootloaderCanId = 0;
            otherBootloaderSerial = 0;
            totalBootloadersDetected = 0;
        }

        private bool TryParseApplicationSerial(string serialNumber, ref bool alreadyInBootloader)
        {
            string cleanSerial = serialNumber.Replace("-", "").PadLeft(10, '0');
            if (ulong.TryParse(cleanSerial, out ulong serial))
            {
                applicationSerial = serial;
                return true;
            }

            if (serialNumber == "BOOTLOADER")
            {
                // Special case: device already in bootloader, serial unknown.
                applicationSerial = NO_SERIAL_VALUE;
                alreadyInBootloader = true;
                return true;
            }

            return false;
        }

        private static bool IsNoSerial(ulong serial)
        {
            return serial == NO_SERIAL_VALUE;
        }

        private void SendBootloaderStatusAck(byte canIdUnit, byte[] sourceMessage)
        {
            if (comPort == null || !comPort.IsOpen || sourceMessage == null || sourceMessage.Length < BOOTLOADER_STATUS_MESSAGE_LENGTH)
            {
                return;
            }

            byte[] response = new byte[8];
            Array.Copy(sourceMessage, response, BOOTLOADER_STATUS_MESSAGE_LENGTH);
            response[7] = BOOTLOADER_STATUS_ACK;
            comPort.WriteCAN(CreateBootloaderControlId(canIdUnit), response);
        }

        private static byte[] CreateBootloaderControlId(byte targetCanId)
        {
            return new byte[] { 0x19, 0xFF, 0x20, targetCanId };
        }

        private static byte ParseTwoDigits(ReadOnlySpan<char> digits, int startIndex)
        {
            return (byte)(((digits[startIndex] - '0') * 10) + (digits[startIndex + 1] - '0'));
        }

        public async Task<bool> UploadFirmwareAsync(
            string port, 
            byte canId, 
            string serialNumber, 
            byte bridgeId, 
            string hexFilePath, 
            bool eraseOnly, 
            bool alreadyInBootloader = false,
            IOperationLogger bootloaderLogger = null,
            string userIdentification = null,
            string originalVersion = null,
            string targetVersion = null,
            string partNumber = null)
        {
            applicationCanId = canId;
            logger = bootloaderLogger;
            logUserIdentification = userIdentification ?? Environment.UserName;
            logOriginalVersion = originalVersion;
            logTargetVersion = targetVersion;
            logApplicationSn = serialNumber;
            logBridgeId = bridgeId.ToString();
            logPartNumber = partNumber;

            ResetOperationState();
            TryParseApplicationSerial(serialNumber, ref alreadyInBootloader);

            if (IsDebugMode())
            {
                ConsoleHelper.WriteInfo($"Connecting to {port}...");
            }

            comPort = new CANPort();
            if (!comPort.Connect(port))
            {
                ConsoleHelper.WriteError("Failed to connect to COM port.");
                return false;
            }

            if (IsDebugMode())
            {
                ConsoleHelper.WriteSuccess("Connected successfully");
            }

            // Subscribe to CAN messages for bootloader status
            comPort.CANPort_MSGReceived += OnBootloaderStatusReceived;

            if (IsSimpleMode())
            {
                // Simple mode: Just show "Updating Status"
                Console.WriteLine();
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("Updating Status:");
                Console.ResetColor();
            }

            // Step 0: Check for existing bootloaders BEFORE entering bootloader mode
            // Only one device should be in bootloader mode at a time
            if (!alreadyInBootloader)
            {
                if (IsDebugMode())
                {
                    ConsoleHelper.WriteInfo("Checking for existing bootloaders on the bus...");
                }

                // Quick scan for bootloader heartbeats (they broadcast every ~500ms)
                var preCheckStart = DateTime.Now;
                while ((DateTime.Now - preCheckStart).TotalMilliseconds < BOOTLOADER_SCAN_TIMEOUT_MS)
                {
                    await Task.Delay(BOOTLOADER_SCAN_POLL_MS);
                    if (totalBootloadersDetected > 0)
                    {
                        ConsoleHelper.WriteError("Another device is already in bootloader mode!");
                        ConsoleHelper.WriteWarning($"Detected bootloader on CAN ID {otherBootloaderCanId} - please wait for it to exit.");
                        CleanupConnection();
                        return false;
                    }
                }
            }

            // Step 1: Enter bootloader mode (SKIP if already in bootloader)
            if (!alreadyInBootloader)
            {
                if (IsAdvancedMode())
                {
                    ConsoleHelper.WriteInfo("Entering bootloader mode...");
                    if (IsDebugMode())
                    {
                        ConsoleHelper.WriteInfo($"Application CAN ID: {applicationCanId}, Serial: {serialNumber}");
                    }
                }

                byte PCID = 0xFE;
                byte[] resetId = { 0x19, 0xEF, canId, PCID };
                byte[] resetMsg = { 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
                comPort.WriteCAN(resetId, resetMsg);

                // Wait for device to enter bootloader mode
                await Task.Delay(BOOTLOADER_ENTRY_DELAY_MS);
            }
            else
            {
                if (IsAdvancedMode())
                {
                    ConsoleHelper.WriteInfo("Device already in bootloader mode, listening for heartbeat...");
                }
                
                // Give a short delay for event handler to be ready
                await Task.Delay(HEARTBEAT_HANDLER_READY_DELAY_MS);
            }

            // Step 2: Wait for bootloader heartbeat
            if (IsDebugMode())
            {
                if (alreadyInBootloader)
                {
                    ConsoleHelper.WriteInfo($"Waiting for bootloader heartbeat on CAN ID {canId}...");
                }
                else
                {
                    ConsoleHelper.WriteInfo($"Scanning for bootloader response (CAN ID may have changed)...");
                }
            }

            bootloaderReady = false;
            var waitStart = DateTime.Now;
            
            // Use shorter timeout for devices already in bootloader (they broadcast every ~500ms)
            double timeoutSeconds = alreadyInBootloader ? HEARTBEAT_WAIT_SECONDS_ALREADY_IN_BOOTLOADER : HEARTBEAT_WAIT_SECONDS_NORMAL;
            
            while (!bootloaderReady && (DateTime.Now - waitStart).TotalSeconds < timeoutSeconds)
            {
                await Task.Delay(HEARTBEAT_WAIT_POLL_MS);
            }

            if (!bootloaderReady)
            {
                if (otherBootloaderDetected)
                {
                    ConsoleHelper.WriteError("Bootloader detected with MISMATCHED serial number!");
                    ConsoleHelper.WriteWarning($"Expected device - CAN ID: {canId}, Serial: {FormatSerial(applicationSerial)}");
                    ConsoleHelper.WriteWarning($"Found bootloader - CAN ID: {otherBootloaderCanId}, Serial: {FormatSerial(otherBootloaderSerial)}");
                    ConsoleHelper.WriteError("The serial number programmed in the bootloader does not match the device serial.");
                    
                    // Check if multiple bootloaders are detected
                    if (totalBootloadersDetected > 1)
                    {
                        ConsoleHelper.WriteError("Multiple devices are in bootloader mode!");
                        ConsoleHelper.WriteWarning("Disconnect the other devices from the bus or try running the program again.");
                        // Exit the bootloader we triggered so it doesn't block the bus
                        await SendExitBootloaderAsync(otherBootloaderCanId, otherBootloaderSerial);
                        CleanupConnection();
                        return false;
                    }
                    
                    // Only one bootloader detected - prompt user to override
                    Console.WriteLine();
                    Console.ForegroundColor = ConsoleColor.Yellow;
                    Console.Write($"Would you like to ignore the mismatch and flash the code on the device with bootloader SN {FormatSerial(otherBootloaderSerial)}? (Y/N): ");
                    Console.ResetColor();
                    
                    var response = Console.ReadKey();
                    Console.WriteLine();
                    
                    if (response.Key != ConsoleKey.Y)
                    {
                        ConsoleHelper.WriteInfo("Firmware update cancelled by user.");
                        // Exit the bootloader so it doesn't block the bus for other operations
                        await SendExitBootloaderAsync(otherBootloaderCanId, otherBootloaderSerial);
                        CleanupConnection();
                        return false;
                    }
                    
                    // User confirmed - accept the mismatched bootloader
                    ConsoleHelper.WriteWarning("Proceeding with serial number mismatch...");
                    
                    // Update our target to match the bootloader we found
                    applicationSerial = otherBootloaderSerial;
                    bootloaderReady = true;
                    bootloaderCanId = otherBootloaderCanId;
                    bootloaderSerial = otherBootloaderSerial;
                }
                else
                {
                    ConsoleHelper.WriteError("Device did not respond to bootloader heartbeat. Check for additional IoT modem on CAN bus.");
                    if (IsDebugMode())
                    {
                        ConsoleHelper.WriteWarning($"No bootloader heartbeat detected within {timeoutSeconds} seconds.");
                        ConsoleHelper.WriteInfo($"Expected CAN ID: {canId}, Serial: {FormatSerial(applicationSerial)}");
                    }
                    CleanupConnection();
                    return false;
                }
            }

            if (IsDebugMode())
            {
                ConsoleHelper.WriteSuccess($"Bootloader responding on CAN ID: {bootloaderCanId}");
            }

            // Step 3: Handle serial number
            byte[] serialBytes;
            
            if (alreadyInBootloader && IsNoSerial(bootloaderSerial))
            {
                // For devices in bootloader with no serial, use dummy serial bytes
                serialBytes = NO_SERIAL_BYTES; // Represents NO_SERIAL_VALUE
                
                if (IsDebugMode())
                {
                    ConsoleHelper.WriteWarning($"Device has no serial number - using default for bootloader operations");
                }
            }
            else if (serialNumber == "BOOTLOADER")
            {
                // Use whatever serial the bootloader reported
                if (IsNoSerial(bootloaderSerial))
                {
                    serialBytes = NO_SERIAL_BYTES;
                }
                else
                {
                    serialBytes = SerialToByteArray(bootloaderSerial);
                }
                
                if (IsAdvancedMode())
                {
                    string serialStr = FormatSerial(bootloaderSerial);
                    ConsoleHelper.WriteInfo($"Using bootloader serial: {serialStr}");
                }
            }
            else
            {
                // Check if bootloader has no serial but we have application serial
                if (IsNoSerial(bootloaderSerial) && !IsNoSerial(applicationSerial))
                {
                    // Copy serial number from application mode to bootloader
                    serialBytes = SerialToByteArray(applicationSerial);
                    
                    if (IsAdvancedMode())
                    {
                        ConsoleHelper.WriteInfo($"Bootloader has no serial - copying from application: {serialNumber}");
                    }

                    WriteBootloaderControlMessage(bootloaderCanId, serialBytes, 0x00, 0x01);
                    await Task.Delay(BOOTLOADER_SERIAL_COPY_DELAY_MS);
                }
                else if (IsNoSerial(bootloaderSerial))
                {
                    // Both bootloader and application have no serial - use dummy
                    serialBytes = NO_SERIAL_BYTES;
                    
                    if (IsDebugMode())
                    {
                        ConsoleHelper.WriteWarning($"Both bootloader and application have no serial number - using default");
                    }
                }
                else
                {
                    // Use bootloader's actual serial number (important for mismatched serial override)
                    serialBytes = SerialToByteArray(bootloaderSerial);
                    
                    if (IsDebugMode())
                    {
                        ConsoleHelper.WriteInfo($"Bootloader already has serial number.");
                    }
                }
            }

            // Step 4: Open bridge for firmware upload
            if (IsAdvancedMode())
            {
                ConsoleHelper.WriteInfo($"Opening Bridge {bridgeId} for firmware upload...");
            }

            WriteBootloaderControlMessage(bootloaderCanId, serialBytes, bridgeId, 0x02);

            // Step 5: Wait for bridge to open
            if (IsDebugMode())
            {
                ConsoleHelper.WriteInfo($"Waiting for bridge {bridgeId} to open (up to 5 seconds)...");
            }

            bridgeOpened = false;
            waitStart = DateTime.Now;
            while (!bridgeOpened && (DateTime.Now - waitStart).TotalSeconds < BRIDGE_OPEN_TIMEOUT_SECONDS)
            {
                await Task.Delay(HEARTBEAT_WAIT_POLL_MS);
            }

            if (!bridgeOpened || currentBridgeId != bridgeId)
            {
                ConsoleHelper.WriteError($"Failed to open Bridge {bridgeId}. Current bridge ID: {currentBridgeId}");
                // Exit bootloader so it doesn't block the bus for next device
                await SendExitBootloaderAsync(bootloaderCanId, bootloaderSerial);
                CleanupConnection();
                return false;
            }

            if (IsDebugMode())
            {
                ConsoleHelper.WriteSuccess($"Bridge {bridgeId} opened successfully");
                int delayMs = BRIDGE_INIT_DELAY_MS;
                ConsoleHelper.WriteInfo($"Waiting for bridge bootloader to fully initialize ({delayMs/1000} seconds)...");
            }

            int initDelayMs = BRIDGE_INIT_DELAY_MS;
            await Task.Delay(initDelayMs);

            // Step 6: Start firmware upload
            // In Simple mode, no message - already showed "Updating Status:"
            if (IsAdvancedMode())
            {
                ConsoleHelper.WriteInfo("Starting firmware upload protocol...");
            }

            protocol = new BootloaderProtocol();
            protocol.CAN_ID = bootloaderCanId;
            protocol.HexFilePath = hexFilePath;
            protocol.COM = comPort;
            protocol.SetEraseOnly(eraseOnly);

            // Wire up event handler for firmware upload protocol
            comPort.CANPort_MSGReceived += protocol.ReceiveCAN;

            // Start the bootloader process
            protocol.ProcessState(BootloaderProtocol.StateMachine.RequestInfo);

            // Start monitoring progress
            cancellationTokenSource = new CancellationTokenSource();
            var monitorTask = MonitorProgressAsync(cancellationTokenSource.Token);

            // Wait for completion
            await monitorTask;

            // Cleanup event handlers
            comPort.CANPort_MSGReceived -= protocol.ReceiveCAN;
            comPort.CANPort_MSGReceived -= OnBootloaderStatusReceived;

            // Step 7: Exit bootloader mode (ALWAYS - both success and failure)
            // This is critical to allow next device in queue to enter bootloader
            try
            {
                if (IsDebugMode())
                {
                    ConsoleHelper.WriteInfo(hasError ? "Exiting bootloader mode after failure..." : "Exiting bootloader mode...");
                }

                WriteBootloaderControlMessage(bootloaderCanId, serialBytes, 0x00, 0x04);
                await Task.Delay(EXIT_BOOTLOADER_DELAY_MS);
            }
            catch (Exception ex)
            {
                // Log but don't fail - we still want to disconnect and continue
                if (IsAdvancedMode())
                {
                    ConsoleHelper.WriteWarning($"Failed to send exit bootloader command: {ex.Message}");
                }
            }

            // Disconnect COM port
            comPort.Disconnect();

            // Log bootload result
            if (logger != null)
            {
                logBootloaderSn = !IsNoSerial(bootloaderSerial)
                    ? bootloaderSerial.ToString().PadLeft(10, '0')
                    : null;
                
                string status = hasError ? "Failed" : "Passed";
                
                await logger.LogBootloadAsync(
                    userIdentification: logUserIdentification,
                    applicationSn: logApplicationSn,
                    bootloaderSn: logBootloaderSn,
                    bridgeId: logBridgeId,
                    originalVersion: logOriginalVersion,
                    updatedToVersion: logTargetVersion,
                    status: status,
                    productType: logPartNumber
                );
            }

            return !hasError;
        }

        private void OnBootloaderStatusReceived(object sender, CANPort.CANEventArgs e)
        {
            if (e.ID_MSG != BOOTLOADER_STATUS_MESSAGE_ID)
            {
                return;
            }

            lock (bootloaderLock)
            {
                if (e.MSG == null || e.MSG.Length < BOOTLOADER_STATUS_MESSAGE_LENGTH)
                {
                    return;
                }

                ulong messageSerial = (ulong)e.MSG[0] * 100000000 +
                                     (ulong)e.MSG[1] * 1000000 +
                                     (ulong)e.MSG[2] * 10000 +
                                     (ulong)e.MSG[3] * 100 +
                                     (ulong)e.MSG[4];

                byte msgProductId = e.MSG[5];
                byte msgBridgeId = e.MSG[6];
                byte status = e.MSG[7];

                currentBridgeId = msgBridgeId;

                if (status != BOOTLOADER_STATUS_READY)
                {
                    return;
                }

                bool isOurDevice = false;

                if (!bootloaderReady)
                {
                    // If we're looking for a device on a specific CAN ID (already in bootloader)
                    if (e.ID_Unit == applicationCanId)
                    {
                        totalBootloadersDetected++;
                        if (IsDebugMode())
                        {
                            string serialStr = FormatSerial(messageSerial);
                            ConsoleHelper.WriteSuccess($"Found bootloader on CAN ID {e.ID_Unit} with serial: {serialStr} (Product ID: {msgProductId}, Bridge ID: {msgBridgeId})");
                        }
                        isOurDevice = true;
                    }
                    // Otherwise check serial number matching
                    else if (IsNoSerial(messageSerial) || messageSerial == applicationSerial)
                    {
                        totalBootloadersDetected++;
                        if (IsDebugMode())
                        {
                            string serialStr = IsNoSerial(messageSerial) ? "NO SERIAL (will be set)" : FormatSerial(messageSerial);
                            ConsoleHelper.WriteSuccess($"Found bootloader on CAN ID {e.ID_Unit} with serial: {serialStr} (Product ID: {msgProductId}, Bridge ID: {msgBridgeId})");
                        }
                        isOurDevice = true;
                    }
                    else if (!otherBootloaderDetected)
                    {
                        // Track mismatched bootloader for better error reporting
                        otherBootloaderDetected = true;
                        otherBootloaderCanId = e.ID_Unit;
                        otherBootloaderSerial = messageSerial;
                        totalBootloadersDetected++;
                        if (IsDebugMode())
                        {
                            string serialStr = FormatSerial(messageSerial);
                            ConsoleHelper.WriteWarning($"Detected bootloader on CAN ID {e.ID_Unit} with DIFFERENT serial: {serialStr} (expected: {FormatSerial(applicationSerial)})");
                        }
                    }
                }
                else if (e.ID_Unit == bootloaderCanId)
                {
                    isOurDevice = true;

                    if (bootloaderReady && !bridgeOpened && msgBridgeId != 0)
                    {
                        bridgeOpened = true;
                    }
                }

                if (isOurDevice)
                {
                    if (!bootloaderReady)
                    {
                        bootloaderReady = true;
                        bootloaderCanId = e.ID_Unit;
                        bootloaderSerial = messageSerial;
                        productId = msgProductId;
                    }

                    SendBootloaderStatusAck(e.ID_Unit, e.MSG);
                }
                else if (otherBootloaderDetected && e.ID_Unit == otherBootloaderCanId)
                {
                    // Keep mismatched bootloader alive while waiting for user decision.
                    SendBootloaderStatusAck(e.ID_Unit, e.MSG);
                }
            }
        }

        // Helper method to convert ulong serial to byte array
        private byte[] SerialToByteArray(ulong serial)
        {
            ReadOnlySpan<char> digits = serial.ToString("D10");
            return new byte[]
            {
                ParseTwoDigits(digits, 0),
                ParseTwoDigits(digits, 2),
                ParseTwoDigits(digits, 4),
                ParseTwoDigits(digits, 6),
                ParseTwoDigits(digits, 8)
            };
        }

        // Helper method to send exit bootloader command (used for early exits after device enters bootloader)
        private async Task SendExitBootloaderAsync(byte targetCanId, ulong targetSerial)
        {
            try
            {
                if (comPort == null || !comPort.IsOpen)
                    return;

                byte[] serialBytes = IsNoSerial(targetSerial)
                    ? NO_SERIAL_BYTES
                    : SerialToByteArray(targetSerial);

                WriteBootloaderControlMessage(targetCanId, serialBytes, 0x00, 0x04);
                await Task.Delay(EARLY_EXIT_BOOTLOADER_DELAY_MS);

                if (IsDebugMode())
                {
                    ConsoleHelper.WriteInfo($"Sent exit bootloader command to CAN ID {targetCanId}");
                }
            }
            catch (Exception ex)
            {
                if (IsDebugMode())
                {
                    ConsoleHelper.WriteWarning($"Failed to send exit bootloader: {ex.Message}");
                }
            }
        }

        private void WriteBootloaderControlMessage(byte targetCanId, byte[] serialBytes, byte bridgeId, byte command)
        {
            if (comPort == null || !comPort.IsOpen || serialBytes == null || serialBytes.Length < 5)
            {
                return;
            }

            byte[] id = CreateBootloaderControlId(targetCanId);
            byte[] msg = new byte[8];
            Array.Copy(serialBytes, 0, msg, 0, 5);
            msg[5] = productId;
            msg[6] = bridgeId;
            msg[7] = command;
            comPort.WriteCAN(id, msg);
        }

        private async Task MonitorProgressAsync(CancellationToken cancellationToken)
        {
            string lastStatus = "";
            int lastProgress = -1;
            bool protocolStarted = false;
            bool showingProgress = false;
            int lastPackageNumber = 0;
            int totalPackagesAtStart = 0; // Store total packages before they get reset

            while (!cancellationToken.IsCancellationRequested)
            {
                // Wait for protocol to start
                if (!protocolStarted)
                {
                    if (protocol.ActiveState == BootloaderProtocol.StateMachine.Idle)
                    {
                        await Task.Delay(PROGRESS_POLL_MS);
                        continue;
                    }

                    protocolStarted = true;
                }

                // Update progress bar
                if (protocol.TotalPackages > 0)
                {
                    // Store total packages when we first see them
                    if (totalPackagesAtStart == 0)
                    {
                        totalPackagesAtStart = protocol.TotalPackages;
                    }

                    int currentProgress = (protocol.PackageNumber * 100) / protocol.TotalPackages;
                    currentProgress = protocol.PackageNumber >= protocol.TotalPackages ? 100 : currentProgress;

                    if (currentProgress != lastProgress || protocol.PackageNumber != lastPackageNumber)
                    {
                        lastProgress = currentProgress;
                        lastPackageNumber = protocol.PackageNumber;
                        showingProgress = true;
                        ConsoleHelper.WriteProgressBar(currentProgress, protocol.PackageNumber, protocol.TotalPackages);
                    }
                }

                // Once started, monitor until it returns to Idle (completion)
                if (protocol.ActiveState == BootloaderProtocol.StateMachine.Idle)
                {
                    if (!isComplete)
                    {
                        // Show final 100% progress using stored total
                        if (totalPackagesAtStart > 0 && lastProgress < 100)
                        {
                            ConsoleHelper.WriteProgressBar(100, totalPackagesAtStart, totalPackagesAtStart);
                        }

                        isComplete = true;

                        // New line after progress bar
                        if (showingProgress)
                        {
                            Console.WriteLine();
                        }

                        if (protocol.Status.Contains("Failed"))
                        {
                            ConsoleHelper.WriteError(protocol.Status);
                            hasError = true;
                        }
                        else
                        {
                            // Success message - show for all modes
                            if (OperatorSettings.CurrentMode == OperatorMode.Simple)
                            {
                                // Simple mode: minimal success message
                                Console.WriteLine();
                            }
                            else if (OperatorSettings.IsAtLeast(OperatorMode.Advanced))
                            {
                                ConsoleHelper.WriteSuccess(protocol.Status);
                            }
                        }
                    }
                    break;
                }

                // Update status if changed (ADVANCED/DEBUG MODE ONLY)
                if (OperatorSettings.IsAtLeast(OperatorMode.Advanced) && protocol.Status != lastStatus)
                {
                    lastStatus = protocol.Status;

                    // Only show intermediate status in Debug mode
                    if (IsDebugMode())
                    {
                        // Clear progress bar line if it exists
                        if (lastProgress >= 0)
                        {
                            Console.WriteLine();
                        }

                        ConsoleHelper.WriteInfo(lastStatus);
                    }
                }

                await Task.Delay(PROGRESS_POLL_MS);
            }
        }
    }
}