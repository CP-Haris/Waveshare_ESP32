using System;
using System.Collections.Generic;
using System.IO;
using System.Timers;

namespace CANBootloaderConsole
{
    public class BootloaderProtocol
    {
        private const uint BOOTLOADER_CAN_ID = 0x19FF22;
        private const byte BOOTLOADER_START_BYTE = 0xAF;
        private const int CAN_DATA_CHUNK_SIZE = 7;
        private const int BOOTLOADER_INFO_RESPONSE_LENGTH = 16;
        private const int FAST_WRITE_TIMEOUT_MS = 500;
        private const int ERASE_TIMEOUT_PER_PAGE_MS = 20;
        private const int ERASE_TIMEOUT_BASE_MS = 1000;

        private bool receivingFrame = false;
        private bool fromFail = false;
        private bool eraseOnly = false;
        private bool stopPressed = false;

        private byte[] hexData;
        private List<byte[]> packData;
        private int packageNumber;
        private int totalPackages;
        private string status = string.Empty;

        public int TotalPackages => totalPackages;
        public int PackageNumber => packageNumber;
        public string Status => status;
        public StateMachine ActiveState => activeState;

        public byte CAN_ID { get; set; }
        public string HexFilePath { get; set; }
        public UnitFirmware Bootloader { get; set; }
        public CANPort COM { get; set; }

        private readonly Timer commTimeoutTimer;
        private readonly int defaultTimeoutMs;
        private byte[] lastSentBuffer;
        private byte lastSentId;
        private StateMachine lastSentState;
        private int retryCount = 0;
        private const int MAX_RETRIES = 5;
        private bool customTimeoutActive = false;

        public enum StateMachine
        {
            Idle,
            RequestInfo,
            GenerateHexData,
            EraseFlash,
            HexDataToPack,
            WriteData,
            WriteDataLast,
            GoToApp,
            Cleanup
        }

        private StateMachine activeState = StateMachine.Idle;
        private StateMachine nextStateFail;
        private StateMachine nextStatePass;
        private readonly object stateLock = new object();

        private static bool IsAdvancedMode() => OperatorSettings.IsAtLeast(OperatorMode.Advanced);

        private static void WriteAdvancedInfo(string message)
        {
            if (IsAdvancedMode())
            {
                ConsoleHelper.WriteInfo(message);
            }
        }

        private static void WriteAdvancedWarning(string message)
        {
            if (IsAdvancedMode())
            {
                ConsoleHelper.WriteWarning(message);
            }
        }

        public BootloaderProtocol()
        {
            Bootloader = new UnitFirmware();
            
            // macOS/Linux may need significantly longer timeout for serial communication
            defaultTimeoutMs = OperatingSystem.IsWindows() ? 2000 : 3000;
            commTimeoutTimer = new Timer(defaultTimeoutMs);
            commTimeoutTimer.Elapsed += OnCommTimeoutElapsed;
            commTimeoutTimer.AutoReset = false;
        }

        private void OnCommTimeoutElapsed(object sender, ElapsedEventArgs e)
        {
            if (retryCount < MAX_RETRIES)
            {
                retryCount++;
                // Only show retries in Advanced/Debug mode
                WriteAdvancedWarning($"Timeout - Retrying ({retryCount}/{MAX_RETRIES})...");
                WriteData(lastSentId, lastSentBuffer);
                RestartCommTimeout();
            }
            else
            {
                // Always show critical errors that cause termination
                ConsoleHelper.WriteError("Maximum retries reached. Aborting operation.");
                fromFail = true;
                ProcessState(StateMachine.Cleanup);
            }
        }

        private void StartCommTimeout(byte id, byte[] buffer, StateMachine state)
        {
            lastSentId = id;
            lastSentBuffer = buffer;
            lastSentState = state;
            commTimeoutTimer.Stop();
            commTimeoutTimer.Interval = defaultTimeoutMs; // Reset to default
            commTimeoutTimer.Start();
        }

        private void StartCommTimeout(byte id, byte[] buffer, StateMachine state, int customTimeoutMs)
        {
            lastSentId = id;
            lastSentBuffer = buffer;
            lastSentState = state;
            commTimeoutTimer.Stop();
            commTimeoutTimer.Interval = customTimeoutMs;
            commTimeoutTimer.Start();
        }

        private void RestartCommTimeout()
        {
            commTimeoutTimer.Stop();
            commTimeoutTimer.Start();
        }

        private void StopCommTimeout()
        {
            commTimeoutTimer.Stop();
        }

        public void SetEraseOnly(bool value) => eraseOnly = value;
        public void Stop() => stopPressed = true;
        public void ClearStop() => stopPressed = false;

        public void Reset()
        {
            WriteData(CAN_ID, BOOTHexDecoder.MSG_Reset());
        }

        public void ReceiveCAN(object sender, CANPort.CANEventArgs e)
        {
            // Fast-path: Filter irrelevant messages BEFORE lock (critical for busy CAN buses)
            if (e.ID_MSG != BOOTLOADER_CAN_ID || e.ID_Unit != CAN_ID || activeState == StateMachine.Idle)
            {
                return;
            }

            lock (stateLock)
            {
                StopCommTimeout();
                if (e.MSG.Length > 1 && e.MSG[1] == BOOTLOADER_START_BYTE && !receivingFrame)
                {
                    receivingFrame = true;
                    Bootloader.CANDataRXBuffer.Clear();
                }

                if (receivingFrame)
                {
                    int numberOfBytes = e.MSG[0];
                    for (int i = 1; i <= numberOfBytes && i < e.MSG.Length; i++)
                        Bootloader.CANDataRXBuffer.Add(e.MSG[i]);

                    if (Bootloader.CANDataRXBuffer.Count == 7)
                    {
                        ushort dataSize = (ushort)(e.MSG[3] | (e.MSG[4] << 8));
                        Bootloader.CANDataRXFrameCount = 7 + dataSize;
                    }

                    if (Bootloader.CANDataRXBuffer.Count == Bootloader.CANDataRXFrameCount)
                    {
                        byte[] msg = Bootloader.CANDataRXBuffer.ToArray();

                        // CRITICAL: Validate minimum message length before accessing Frame constants
                        if (msg.Length < Frame.DATA)
                        {
                            WriteAdvancedWarning($"Received message too short: {msg.Length} bytes (expected at least {Frame.DATA})");
                            Bootloader.CANDataRXBuffer.Clear();
                            receivingFrame = false;
                            return;
                        }

                        // Use protocol constants for message parsing
                        switch (msg[Frame.CMD])
                        {
                            case Command.READ_VERSION:
                                // Validate we have enough data for version info
                                if (msg.Length < Data_Info.PROGRAM_FLASH_END + 4)
                                {
                                    WriteAdvancedWarning($"Version response too short: {msg.Length} bytes");
                                    break;
                                }

                                ushort dataLength = BitConverter.ToUInt16(msg, Frame.DATA_LENGTH);
                                if (dataLength == BOOTLOADER_INFO_RESPONSE_LENGTH)
                                {
                                    // Only show in Advanced/Debug mode
                                    WriteAdvancedInfo("Bootloader info received");
                                    
                                    if (OperatorSettings.CurrentMode == OperatorMode.Advanced)
                                    {
                                        ConsoleHelper.WriteInfo($"    Full message: {BitConverter.ToString(msg)}");
                                    }
                                    
                                    Bootloader.version = BitConverter.ToUInt16(msg, Data_Info.VERSION);
                                    Bootloader.device_id = BitConverter.ToUInt16(msg, Data_Info.DEVICE_ID);
                                    Bootloader.erase_page_size = BitConverter.ToUInt16(msg, Data_Info.ERASE_PAGE_SIZE);
                                    Bootloader.minimum_write_block_size = BitConverter.ToUInt16(msg, Data_Info.MINIMUM_WRITE_BLOCK_SIZE);
                                    Bootloader.program_flash_start = BitConverter.ToUInt32(msg, Data_Info.PROGRAM_FLASH_START);
                                    Bootloader.program_flash_end = BitConverter.ToUInt32(msg, Data_Info.PROGRAM_FLASH_END);
                                    
                                    // Only show details in Debug mode
                                    if (OperatorSettings.CurrentMode == OperatorMode.Advanced)
                                    {
                                        ConsoleHelper.WriteInfo($"  Device ID: 0x{Bootloader.device_id:X4}");
                                        ConsoleHelper.WriteInfo($"  Flash: 0x{Bootloader.program_flash_start:X8} - 0x{Bootloader.program_flash_end:X8}");
                                        ConsoleHelper.WriteInfo($"  Erase page size: {Bootloader.erase_page_size} bytes");
                                    }
                                    
                                    // Validate bootloader parameters (always check, always show errors)
                                    if (Bootloader.erase_page_size == 0)
                                    {
                                        ConsoleHelper.WriteError("Invalid erase page size (0). Cannot proceed.");
                                        fromFail = true;
                                        ProcessState(StateMachine.Cleanup);
                                        return;
                                    }
                                    
                                    if (Bootloader.program_flash_end <= Bootloader.program_flash_start)
                                    {
                                        ConsoleHelper.WriteError($"Invalid flash range: 0x{Bootloader.program_flash_start:X8} - 0x{Bootloader.program_flash_end:X8}");
                                        fromFail = true;
                                        ProcessState(StateMachine.Cleanup);
                                        return;
                                    }
                                }
                                else
                                {
                                    WriteAdvancedWarning($"Unexpected version data length: {dataLength} (expected {BOOTLOADER_INFO_RESPONSE_LENGTH})");
                                }
                                break;
                                
                            case Command.WRITE_FLASH:
                            case Command.ERASE_FLASH:
                                // Erase/Write confirmation - status is handled below
                                if (IsAdvancedMode() && msg[Frame.CMD] == Command.ERASE_FLASH)
                                {
                                    ConsoleHelper.WriteInfo("Erase operation completed");
                                }
                                break;
                                
                            case Command.READ_FLASH:
                            case Command.CALC_CHECKSUM:
                            case Command.RESET_DEVICE:
                                // These commands don't have special data to parse - status is handled below
                                break;
                                
                            default:
                                // Only log truly unexpected commands in Debug mode
                                if (msg[Frame.CMD] > Command.RESET_DEVICE && OperatorSettings.CurrentMode == OperatorMode.Advanced)
                                {
                                    ConsoleHelper.WriteWarning($"Received unexpected command: 0x{msg[Frame.CMD]:X2}");
                                }
                                break;
                        }

                        // Read status from correct position using Frame.STATUS constant
                        if (msg.Length > Frame.STATUS)
                        {
                            Bootloader.status = (UnitFirmware.Errorcode)msg[Frame.STATUS];
                        }
                        else
                        {
                            // Always show critical errors
                            ConsoleHelper.WriteError($"Message too short to read status byte (length: {msg.Length})");
                            Bootloader.CANDataRXBuffer.Clear();
                            receivingFrame = false;
                            return;
                        }

                        if (stopPressed)
                        {
                            fromFail = true;
                            ProcessState(StateMachine.Cleanup);
                        }
                        else if (Bootloader.status == UnitFirmware.Errorcode.BOOTLOADER_CMD_RESPONSE_COMMAND_SUCCESS)
                        {
                            if (activeState != StateMachine.Idle)
                            {
                                retryCount = 0;
                                fromFail = false;
                                ProcessState(nextStatePass);
                            }
                        }
                        else if (Bootloader.status == UnitFirmware.Errorcode.BOOTLOADER_CMD_RESPONSE_BAD_CRC)
                        {
                            // Only show in Advanced/Debug mode
                            WriteAdvancedWarning("Bad CRC detected in response");
                            fromFail = true;
                            ProcessState(nextStateFail);
                        }
                        else if (Bootloader.status > UnitFirmware.Errorcode.BOOTLOADER_CMD_RESPONSE_COMMAND_SUCCESS)
                        {
                            // Provide more detailed error messages - only in Advanced/Debug or if critical
                            string errorMsg = Bootloader.status switch
                            {
                                UnitFirmware.Errorcode.BOOTLOADER_CMD_RESPONSE_UNSUPPORTED_COMMAND => "Unsupported command",
                                UnitFirmware.Errorcode.BOOTLOADER_CMD_RESPONSE_BAD_ADDRESS => "Bad memory address",
                                UnitFirmware.Errorcode.BOOTLOADER_CMD_RESPONSE_BAD_LENGTH => "Bad data length",
                                UnitFirmware.Errorcode.BOOTLOADER_CMD_RESPONSE_VERIFY_FAIL => "Verification failed",
                                UnitFirmware.Errorcode.BOOTLOADER_CMD_RESPONSE_BAD_CRC => "Bad CRC",
                                UnitFirmware.Errorcode.BOOTLOADER_CMD_RESPONSE_TIMEOUT => "Bootloader timeout",
                                _ => $"Unknown error code: 0x{msg[Frame.STATUS]:X2}"
                            };
                            
                            // Show warnings in Advanced/Debug mode
                            WriteAdvancedWarning($"Bootloader error: {errorMsg} (0x{msg[Frame.STATUS]:X2})");
                            fromFail = true;
                            ProcessState(nextStateFail);
                        }

                        Bootloader.CANDataRXBuffer.Clear();
                        receivingFrame = false;
                    }
                    else if (Bootloader.CANDataRXBuffer.Count > Bootloader.CANDataRXFrameCount)
                    {
                        WriteAdvancedWarning($"Buffer overflow: {Bootloader.CANDataRXBuffer.Count} > {Bootloader.CANDataRXFrameCount}");
                        Bootloader.CANDataRXBuffer.Clear();
                        receivingFrame = false;
                    }
                }
            }
        }

        public void WriteData(byte id, byte[] buffer)
        {
            lastSentId = id;
            lastSentBuffer = buffer;
            lastSentState = activeState;

            try
            {
                if (COM == null)
                    throw new InvalidOperationException("COM property must be set before using BootloaderProtocol.");
                    
                if (!COM.IsOpen)
                {
                    // Always show critical errors
                    ConsoleHelper.WriteError("Port is not open. Cannot send CAN message.");
                    fromFail = true;
                    ProcessState(StateMachine.Cleanup);
                    return;
                }

                for (int i = 0; i < buffer.Length; i += CAN_DATA_CHUNK_SIZE)
                {
                    byte[] canId = { 0x19, 0xFF, 0x21, id };
                    byte[] canMsg = new byte[8];

                    int bytes = Math.Min(CAN_DATA_CHUNK_SIZE, buffer.Length - i);
                    canMsg[0] = (byte)bytes;

                    for (int a = 0; a < bytes; a++)
                        canMsg[a + 1] = buffer[i + a];

                    COM.WriteCAN(canId, canMsg);
                }

                if (activeState == StateMachine.WriteData || activeState == StateMachine.WriteDataLast ||
                    activeState == StateMachine.RequestInfo || activeState == StateMachine.EraseFlash)
                {
                    // Only start timeout if custom timeout is not already active
                    if (!customTimeoutActive)
                    {
                        // Use fast timeout for write operations, normal timeout for others
                        if (activeState == StateMachine.WriteData || activeState == StateMachine.WriteDataLast)
                        {
                            StartCommTimeout(id, buffer, activeState, FAST_WRITE_TIMEOUT_MS); // Fast retry for data writes
                        }
                        else
                        {
                            StartCommTimeout(id, buffer, activeState); // Normal timeout for RequestInfo
                        }
                    }
                }
            }
            catch (InvalidOperationException ex)
            {
                // Always show critical errors
                ConsoleHelper.WriteError($"COM port error: {ex.Message}");
                fromFail = true;
                ProcessState(StateMachine.Cleanup);
            }
            catch (Exception ex)
            {
                // Always show critical errors
                ConsoleHelper.WriteError($"Unexpected error sending data: {ex.Message}");
                fromFail = true;
                ProcessState(StateMachine.Cleanup);
            }
        }

        public void ProcessState(StateMachine state)
        {
            lock (stateLock)
            {
                activeState = state;

            while (true)
            {
                if (stopPressed && activeState != StateMachine.Cleanup)
                {
                    activeState = StateMachine.Cleanup;
                }

                switch (activeState)
                {
                    case StateMachine.RequestInfo:
                        status = "Requesting Bootloader Info...";
                        byte[] requestMsg = BOOTHexDecoder.MSG_Request();
                        if (OperatorSettings.CurrentMode == OperatorMode.Advanced)
                        {
                            ConsoleHelper.WriteInfo($"Sending MSG_Request to CAN ID: 0x{CAN_ID:X2}");
                            ConsoleHelper.WriteInfo($"    MSG_Request payload: {BitConverter.ToString(requestMsg)}");
                            ConsoleHelper.WriteInfo($"    Expected response: ID_MSG=0x{BOOTLOADER_CAN_ID:X}, ID_Unit=0x{CAN_ID:X2}");
                        }
                        WriteData(CAN_ID, requestMsg);
                        nextStatePass = StateMachine.GenerateHexData;
                        nextStateFail = StateMachine.Cleanup;
                        return;

                    case StateMachine.GenerateHexData:
                        status = "Generating HEX Data...";
                        try
                        {
                            string hexText = File.ReadAllText(HexFilePath);
                            hexData = BOOTHexDecoder.HexFileToData(hexText, Bootloader.program_flash_start, Bootloader.program_flash_end);
                            
                            // Validate hex data
                            if (hexData == null || hexData.Length == 0)
                            {
                                ConsoleHelper.WriteError("Generated hex data is empty");
                                fromFail = true;
                                activeState = StateMachine.Cleanup;
                                continue;
                            }
                            
                            // Only show in Advanced/Debug mode
                            WriteAdvancedInfo($"Generated {hexData.Length} bytes of firmware data");
                        }
                        catch (FileNotFoundException ex)
                        {
                            // Always show critical errors
                            ConsoleHelper.WriteError($"Hex file not found: {ex.Message}");
                            fromFail = true;
                            activeState = StateMachine.Cleanup;
                            continue;
                        }
                        catch (UnauthorizedAccessException ex)
                        {
                            // Always show critical errors
                            ConsoleHelper.WriteError($"Access denied to hex file: {ex.Message}");
                            fromFail = true;
                            activeState = StateMachine.Cleanup;
                            continue;
                        }
                        catch (Exception ex)
                        {
                            // Always show critical errors
                            ConsoleHelper.WriteError($"Failed to read hex file: {ex.Message}");
                            fromFail = true;
                            activeState = StateMachine.Cleanup;
                            continue;
                        }
                        nextStatePass = StateMachine.EraseFlash;
                        nextStateFail = StateMachine.EraseFlash;
                        activeState = StateMachine.EraseFlash;
                        continue;

                    case StateMachine.EraseFlash:
                        status = "Erasing Device...";
                        
                        // Validate erase parameters
                        if (Bootloader.erase_page_size == 0)
                        {
                            // Always show critical errors
                            ConsoleHelper.WriteError("Cannot erase: invalid page size");
                            fromFail = true;
                            activeState = StateMachine.Cleanup;
                            continue;
                        }
                        
                        uint pagesToErase = (uint)Math.Ceiling((double)hexData.Length / Bootloader.erase_page_size);
                        
                        // Only show in Advanced/Debug mode
                        WriteAdvancedInfo($"Erasing {pagesToErase} pages...");
                        
                        // Flash erase takes ~20ms per page + 1 second overhead
                        int eraseTimeoutMs = (int)(pagesToErase * ERASE_TIMEOUT_PER_PAGE_MS) + ERASE_TIMEOUT_BASE_MS;
                        byte[] eraseMsg = BOOTHexDecoder.MSG_EraseFlash(Bootloader.program_flash_start, pagesToErase);
                        
                        // Set custom timeout before WriteData
                        customTimeoutActive = true;
                        StartCommTimeout(CAN_ID, eraseMsg, StateMachine.EraseFlash, eraseTimeoutMs);
                        retryCount = 0;
                        
                        // WriteData calls COM.Write internally
                        WriteData(CAN_ID, eraseMsg);
                        customTimeoutActive = false;
                        nextStatePass = eraseOnly ? StateMachine.Cleanup : StateMachine.HexDataToPack;
                        nextStateFail = StateMachine.Cleanup;
                        return;

                    case StateMachine.HexDataToPack:
                        status = "Sorting Firmware Data...";
                        try
                        {
                            packData = BOOTHexDecoder.MSG_DataToSend(hexData, Bootloader.program_flash_start, 240);
                            
                            if (packData == null || packData.Count == 0)
                            {
                                // Always show critical errors
                                ConsoleHelper.WriteError("Failed to create firmware packages");
                                fromFail = true;
                                activeState = StateMachine.Cleanup;
                                continue;
                            }
                            
                            packageNumber = 0;
                            totalPackages = packData.Count;
                            
                            // Only show in Advanced/Debug mode
                            WriteAdvancedInfo($"Created {totalPackages} firmware packages");
                        }
                        catch (Exception ex)
                        {
                            // Always show critical errors
                            ConsoleHelper.WriteError($"Failed to prepare firmware data: {ex.Message}");
                            fromFail = true;
                            activeState = StateMachine.Cleanup;
                            continue;
                        }
                        nextStatePass = StateMachine.WriteData;
                        nextStateFail = StateMachine.WriteData;
                        activeState = StateMachine.WriteData;
                        continue;

                    case StateMachine.WriteData:
                        status = $"Uploading Firmware...";
                        if (!fromFail)
                            packageNumber++;

                        if (packageNumber < packData.Count)
                        {
                            WriteData(CAN_ID, packData[packageNumber]);
                            nextStatePass = StateMachine.WriteData;
                            nextStateFail = StateMachine.WriteData;
                            return;
                        }
                        else
                        {
                            nextStatePass = StateMachine.WriteDataLast;
                            nextStateFail = StateMachine.WriteDataLast;
                            activeState = StateMachine.WriteDataLast;
                            continue;
                        }

                    case StateMachine.WriteDataLast:
                        status = $"Finalizing Upload...";
                        packageNumber = 0;
                        WriteData(CAN_ID, packData[packageNumber]);
                        nextStatePass = StateMachine.GoToApp;
                        nextStateFail = StateMachine.WriteDataLast;
                        return;

                    case StateMachine.GoToApp:
                        status = "Resetting Device...";
                        Reset();
                        nextStatePass = StateMachine.Cleanup;
                        nextStateFail = StateMachine.GoToApp;
                        return;

                    case StateMachine.Cleanup:
                        // Stop timeout timer if still running
                        StopCommTimeout();
                        
                        if (fromFail)
                        {
                            status = "Bootloading Failed";
                            // Always show failure messages
                            ConsoleHelper.WriteError($"Firmware upload failed at state: {lastSentState}");
                        }
                        else
                        {
                            status = "Bootloading Completed";
                        }
                        eraseOnly = false;
                        stopPressed = false;

                        totalPackages = 1;
                        retryCount = 0;
                        activeState = StateMachine.Idle;
                        return;

                    default:
                        WriteAdvancedWarning($"Unknown state: {activeState}");
                        return;
                }
            }
            }
        }
    }
}