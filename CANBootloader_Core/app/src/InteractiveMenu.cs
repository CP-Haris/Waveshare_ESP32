using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using CANBootloaderConsole.DeviceScanner;

namespace CANBootloaderConsole
{
    public class InteractiveMenu
    {
        private const int STARTUP_DELAY_SIMPLE_MS = 2000;
        private const int STARTUP_DELAY_ADVANCED_MS = 1500;
        private const int MODE_TOGGLE_DELAY_MS = 1000;

        private readonly FirmwareCache cache;
        private readonly IFirmwareSource repository;
        private readonly IOperationLogger logger;
        private List<CANDevice> discoveredDevices;
        private List<BootloaderDevice> bootloaderDevices;
        private string currentComPortName; // Stored for use between batch operations
        private string startupSyncStatusMessage;

        public InteractiveMenu(FirmwareCache firmwareCache, IFirmwareSource firmwareRepo, IOperationLogger bootloaderLogger = null)
        {
            cache = firmwareCache;
            repository = firmwareRepo;
            logger = bootloaderLogger;
        }

        // Simple struct to hold bootloader device info
        private class BootloaderDevice
        {
            public byte CanId { get; set; }
            public byte DeviceId { get; set; }
            public ushort BootloaderVersion { get; set; }
            public ulong SerialNumber { get; set; }
            public byte CurrentBridgeId { get; set; }
        }

        public async Task RunAsync()
        {
            // Migrate any old unencrypted cache files on startup
            cache.MigrateUnencryptedFiles();

            // Update cache from configured firmware source on startup
            await UpdateCacheFromSourceAsync(isStartupSync: true);
            
            // Give user time to read the startup messages
            int startupDelayMs = OperatorSettings.CurrentMode == OperatorMode.Simple
                ? STARTUP_DELAY_SIMPLE_MS
                : STARTUP_DELAY_ADVANCED_MS;
            await Task.Delay(startupDelayMs);

            while (true)
            {
                ShowMainMenu();
                var choice = Console.ReadKey(true);
                Console.WriteLine();

                await HandleMenuChoiceAsync(choice.Key);

                if (choice.Key == ConsoleKey.Q || choice.Key == ConsoleKey.Escape)
                {
                    return;
                }
            }
        }

        private Task HandleMenuChoiceAsync(ConsoleKey key)
        {
            return OperatorSettings.CurrentMode == OperatorMode.Simple
                ? HandleSimpleMenuChoiceAsync(key)
                : HandleAdvancedMenuChoiceAsync(key);
        }

        private Task HandleSimpleMenuChoiceAsync(ConsoleKey key)
        {
            return key switch
            {
                ConsoleKey.D1 or ConsoleKey.NumPad1 => UpdateFirmwareAsync(),
                ConsoleKey.D2 or ConsoleKey.NumPad2 => ReloadCurrentFirmwareAsync(),
                ConsoleKey.R => ResetFirmwareCacheAsync(),
#if ENABLE_ADVANCED_MODE
                ConsoleKey.M => ToggleOperatorModeAsync(),
#endif
                ConsoleKey.Q or ConsoleKey.Escape => ExitApplicationAsync(),
                _ => InvalidOptionAsync()
            };
        }

        private Task HandleAdvancedMenuChoiceAsync(ConsoleKey key)
        {
            return key switch
            {
                ConsoleKey.D1 or ConsoleKey.NumPad1 => ScanDevicesAsync(),
                ConsoleKey.D2 or ConsoleKey.NumPad2 => UpdateFirmwareAsync(),
                ConsoleKey.D3 or ConsoleKey.NumPad3 => UpdateCacheFromSourceAsync(),
                ConsoleKey.D4 or ConsoleKey.NumPad4 => ShowCachedFirmwareAsync(),
                ConsoleKey.D5 or ConsoleKey.NumPad5 => ListAvailablePortsAsync(),
                ConsoleKey.R => ResetFirmwareCacheAsync(),
#if ENABLE_ADVANCED_MODE
                ConsoleKey.M => ToggleOperatorModeAsync(),
#endif
                ConsoleKey.Q or ConsoleKey.Escape => ExitApplicationAsync(),
                _ => InvalidOptionAsync()
            };
        }

        private Task InvalidOptionAsync()
        {
            ConsoleHelper.WriteWarning("Invalid option. Please try again.");
            return Task.CompletedTask;
        }

        private Task ShowCachedFirmwareAsync()
        {
            ShowCachedFirmware();
            return Task.CompletedTask;
        }

        private Task ListAvailablePortsAsync()
        {
            CANPort.ListAvailablePorts();
            PressAnyKey();
            return Task.CompletedTask;
        }

        private Task ExitApplicationAsync()
        {
            ConsoleHelper.WriteInfo("Exiting application...");
            return Task.CompletedTask;
        }

        private void ShowMainMenu()
        {
            try
            {
                Console.Clear();
            }
            catch (System.IO.IOException)
            {
                // Console.Clear() not supported in this environment (e.g., VS Debug Console)
                // Print newlines instead to create visual separation
                Console.WriteLine("\n\n\n");
            }
            
            ConsoleHelper.WriteHeader("CAN Bootloader - Main Menu");
            if (!string.IsNullOrWhiteSpace(startupSyncStatusMessage))
            {
                Console.ForegroundColor = ConsoleColor.DarkGray;
                Console.WriteLine($"  {startupSyncStatusMessage}");
                Console.ResetColor();
                Console.WriteLine();
            }
            
            // Build menu items based on operator mode
            var menuItems = new List<(string key, string description, ConsoleColor color)>();

            if (OperatorSettings.CurrentMode == OperatorMode.Simple)
            {
                // Simple mode: Only essential options
                menuItems.Add(("1", "Update Firmware", ConsoleColor.Cyan));
                menuItems.Add(("2", "Reload Current Firmware", ConsoleColor.Cyan));
                menuItems.Add(("R", "Reset Firmware Cache", ConsoleColor.Cyan));
#if ENABLE_ADVANCED_MODE
                menuItems.Add(("M", "Change to Advanced Mode", ConsoleColor.Magenta));
#endif
                menuItems.Add(("Q", "Quit", ConsoleColor.Yellow));
            }
            else
            {
                // Advanced mode: All options
                menuItems.Add(("1", "Scan CAN Bus for Devices", ConsoleColor.Cyan));
                menuItems.Add(("2", "Update Firmware", ConsoleColor.Cyan));
                menuItems.Add(("3", "Update Firmware Cache", ConsoleColor.Cyan));
                menuItems.Add(("4", "View Cached Firmware", ConsoleColor.Cyan));
                menuItems.Add(("5", "List COM Ports", ConsoleColor.Cyan));
                menuItems.Add(("R", "Reset Firmware Cache", ConsoleColor.Cyan));
#if ENABLE_ADVANCED_MODE
                menuItems.Add(("M", "Change to Simple Mode", ConsoleColor.Magenta));
#endif
                menuItems.Add(("Q", "Quit", ConsoleColor.Yellow));
            }

            foreach (var (key, description, color) in menuItems)
            {
                Console.ForegroundColor = color;
                Console.WriteLine($"  {key}. {description}");
            }

            Console.ResetColor();
            Console.WriteLine();
            Console.Write("  Select an option: ");
        }

        private async Task ToggleOperatorModeAsync()
        {
            if (OperatorSettings.CurrentMode == OperatorMode.Simple)
            {
                OperatorSettings.CurrentMode = OperatorMode.Advanced;
                ConsoleHelper.WriteSuccess("Switched to Advanced Mode");
            }
            else
            {
                OperatorSettings.CurrentMode = OperatorMode.Simple;
                ConsoleHelper.WriteSuccess("Switched to Simple Mode");
            }

            await Task.Delay(MODE_TOGGLE_DELAY_MS);
        }

        private async Task UpdateCacheFromSourceAsync(bool isStartupSync = false)
        {
            string sourceName = repository is ApiFirmwareSource ? "API" : "firmware source";

            // Only show header in Advanced/Debug mode
            if (OperatorSettings.IsAtLeast(OperatorMode.Advanced))
            {
                ConsoleHelper.WriteHeader($"Updating Cache from {sourceName}");
            }
            else
            {
                // Simple mode: Just show a brief status
                ConsoleHelper.WriteInfo("Checking for firmware updates...");
            }

            if (repository == null)
            {
                if (isStartupSync)
                    startupSyncStatusMessage = "Startup check: offline mode (using local cache only).";

                if (OperatorSettings.IsAtLeast(OperatorMode.Advanced))
                {
                    ConsoleHelper.WriteWarning("Data source not configured. Working in offline mode.");
                    PressAnyKey();
                }
                return;
            }

            // Avoid a separate startup health probe to reduce startup latency.
            // We use the catalog fetch itself as the connectivity test.
            bool shouldProbeConnectivity = OperatorSettings.IsAtLeast(OperatorMode.Advanced) && !isStartupSync;
            bool connectionOk = true;
            if (shouldProbeConnectivity)
            {
                connectionOk = await repository.TestConnectionAsync();
                if (!connectionOk)
                {
                    ConsoleHelper.WriteWarning($"Connectivity check to {sourceName} failed. Attempting firmware fetch anyway...");
                }
            }

            // GetAllFirmwareAsync now handles its own mode-aware output
            var sourceFirmware = await repository.GetAllFirmwareAsync(cache.GetAllFirmware());
            
            if (sourceFirmware.Count == 0)
            {
                if (isStartupSync)
                {
                    startupSyncStatusMessage = "Startup check: no firmware updates found or source unavailable. Using local cache.";
                }

                if (OperatorSettings.IsAtLeast(OperatorMode.Advanced))
                {
                    ConsoleHelper.WriteWarning(connectionOk
                        ? "No firmware found in source."
                        : $"Cannot connect to {sourceName}. Working in offline mode with cached firmware.");
                    PressAnyKey();
                }
                return;
            }

            int added = 0;
            int updated = 0;
            int skipped = 0;

            foreach (var firmware in sourceFirmware)
            {
                var exactMatch = cache.GetAllFirmwareVersions(firmware.PartNumber, firmware.BridgeId)
                                      .FirstOrDefault(f => f.Version == firmware.Version);

                if (exactMatch == null)
                {
                    if (OperatorSettings.IsAtLeast(OperatorMode.Advanced))
                        ConsoleHelper.WriteInfo($"Adding: {firmware.PartNumber} Bridge {firmware.BridgeId} V{firmware.VersionString}");
                    cache.AddOrUpdateFirmware(firmware);
                    added++;
                }
                else if (exactMatch.LastUpdated < firmware.LastUpdated)
                {
                    if (OperatorSettings.IsAtLeast(OperatorMode.Advanced))
                        ConsoleHelper.WriteInfo($"Updating: {firmware.PartNumber} Bridge {firmware.BridgeId} V{firmware.VersionString}");
                    cache.AddOrUpdateFirmware(firmware);
                    updated++;
                }
                else
                {
                    skipped++;
                }
            }

            // Simple mode: Clean summary
            if (OperatorSettings.CurrentMode == OperatorMode.Simple)
            {
                if (added > 0 || updated > 0)
                {
                    ConsoleHelper.WriteSuccess($"Firmware cache updated ({added + updated} new versions)");
                    if (isStartupSync)
                        startupSyncStatusMessage = $"Startup check: cache updated ({added + updated} new versions).";
                }
                else
                {
                    ConsoleHelper.WriteSuccess("Firmware cache is up to date");
                    if (isStartupSync)
                        startupSyncStatusMessage = "Startup check: firmware cache is up to date.";
                }
            }
            else
            {
                if (isStartupSync)
                {
                    if (added > 0 || updated > 0)
                        startupSyncStatusMessage = $"Startup check: cache updated ({added + updated} new versions).";
                    else
                        startupSyncStatusMessage = "Startup check: firmware cache is up to date.";
                }

                // Advanced/Debug mode: Detailed summary
                Console.WriteLine();
                Console.ForegroundColor = ConsoleColor.White;
                Console.WriteLine("  Cache Sync Summary");
                Console.WriteLine("  " + new string('-', 40));
                Console.ResetColor();
                
                if (added > 0)
                {
                    Console.ForegroundColor = ConsoleColor.Green;
                    Console.WriteLine($"  \u2713 Added:       {added,3} new firmware versions");
                }
                if (updated > 0)
                {
                    Console.ForegroundColor = ConsoleColor.Cyan;
                    Console.WriteLine($"  \u21BB Updated:    {updated,3} firmware versions");
                }
                Console.ForegroundColor = ConsoleColor.Gray;
                Console.WriteLine($"  \u25E6 Already current: {skipped,3} firmware versions");
                Console.ResetColor();
                Console.WriteLine();
                
                PressAnyKey();
            }
        }

        /// <summary>
        /// Resets firmware cache by reloading all files from the active data source.
        /// </summary>
        private async Task ResetFirmwareCacheAsync()
        {
            ConsoleHelper.WriteHeader("Reset Firmware Cache");
            
            Console.WriteLine();
            ConsoleHelper.WriteWarning("This will replace all cached firmware with fresh copies from the active data source.");
            Console.Write("  Continue? (Y/N): ");

            // Confirm reset
            bool confirmed = false;
            while (true)
            {
                var key = Console.ReadKey(true);

                if (key.Key == ConsoleKey.Y)
                {
                    Console.WriteLine("Y");
                    confirmed = true;
                    break;
                }
                else if (key.Key == ConsoleKey.N || key.Key == ConsoleKey.Escape)
                {
                    Console.WriteLine("N");
                    confirmed = false;
                    break;
                }
            }

            if (!confirmed)
            {
                ConsoleHelper.WriteInfo("Reset cancelled.");
                PressAnyKey();
                return;
            }

            if (repository == null)
            {
                ConsoleHelper.WriteWarning("No data source configured. Cache reset aborted to avoid data loss.");
                PressAnyKey();
                return;
            }

            Console.WriteLine();

            // Step 1: Fetch fresh firmware before deleting local cache.
            ConsoleHelper.WriteInfo("Downloading firmware from data source...");
            var freshFirmware = await repository.GetAllFirmwareAsync();

            if (freshFirmware.Count == 0)
            {
                ConsoleHelper.WriteWarning("No firmware could be downloaded. Existing cache was not modified.");
                PressAnyKey();
                return;
            }

            // Step 2: Replace local cache only after successful download.
            ConsoleHelper.WriteInfo("Clearing existing cache...");
            cache.ClearCache();

            int imported = 0;
            foreach (var firmware in freshFirmware)
            {
                cache.AddOrUpdateFirmware(firmware);
                imported++;
            }

            Console.WriteLine();
            ConsoleHelper.WriteSuccess($"Cache reset complete. {imported} firmware files imported.");
            
            PressAnyKey();
        }

        private async Task<List<CANDevice>> EnsureDevicesScannedAsync()
        {
            // Return existing devices if available
            if (discoveredDevices?.Count > 0)
                return discoveredDevices;

            // Try to find port with either VID/PID combination
            var comPortName = CANPort.FindPortByVidPid(
                "0483", "5740",  // CP - IOT Modem
                "04D8", "000A"   // CP - CAN Modem
            );

            if (string.IsNullOrEmpty(comPortName))
            {
                ConsoleHelper.WriteError("No compatible COM port found.");
                if (OperatorSettings.IsAtLeast(OperatorMode.Advanced))
                {
                    ConsoleHelper.WriteInfo("Please ensure a CAN adapter is connected.");
                    ConsoleHelper.WriteInfo("Supported devices:");
                    ConsoleHelper.WriteInfo("  - CP IOT Modem (VID:0483 PID:5740)");
                    ConsoleHelper.WriteInfo("  - CP CAN Modem (VID:04D8 PID:000A)");
                }
                return null;
            }

            // Store COM port name for use in batch operations
            currentComPortName = comPortName;

            // FIRST: Check for bootloader heartbeats (devices stuck in bootloader mode)
            bootloaderDevices = await ScanForBootloaderDevicesAsync(comPortName);

            // If we found devices in bootloader mode, handle them first
            if (bootloaderDevices?.Count > 0)
            {
                bool handled = await HandleBootloaderDevicesAsync(comPortName);
                if (!handled)
                {
                    // User chose to cancel
                    return null;
                }
            }

            // SECOND: Scan for normal devices
            if (OperatorSettings.IsAtLeast(OperatorMode.Advanced))
            {
                ConsoleHelper.WriteInfo("Scanning CAN bus for devices...");
                Console.WriteLine();
            }
            else
            {
                ConsoleHelper.WriteInfo("Scanning CAN bus...");
            }

            var scanner = new DeviceScanner.DeviceScanner();
            
            discoveredDevices = await scanner.ScanDevicesAsync(comPortName);

            if (discoveredDevices?.Count > 0)
            {
                DeviceScanner.DeviceScanner.DisplayDevices(discoveredDevices);
            }
            else
            {
                ConsoleHelper.WriteWarning("No devices found on CAN bus.");
                if (OperatorSettings.IsAtLeast(OperatorMode.Advanced))
                    ConsoleHelper.WriteInfo("Please ensure devices are powered on and connected.");
            }

            return discoveredDevices;
        }

        private async Task<List<BootloaderDevice>> ScanForBootloaderDevicesAsync(string comPortName)
        {
            var bootloaderDevs = new List<BootloaderDevice>();
            var responseLock = new object();

            try
            {
                using (var port = new CANPort())
                {
                    if (!port.Connect(comPortName))
                        return bootloaderDevs;

                // Subscribe to CAN messages - look for bootloader heartbeat (0x19FF20)
                port.CANPort_MSGReceived += (sender, e) =>
                {
                    // Look for bootloader heartbeat messages (0x19FF20 with status 199)
                    if (e.ID_MSG == 0x19FF20 && e.MSG.Length >= 8)
                    {
                        byte status = e.MSG[7];
                        
                        // Status 199 means device is in bootloader mode waiting for commands
                        if (status == 199)
                        {
                            // Parse serial number from heartbeat
                            ulong serial = (ulong)e.MSG[0] * 100000000 +
                                         (ulong)e.MSG[1] * 1000000 +
                                         (ulong)e.MSG[2] * 10000 +
                                         (ulong)e.MSG[3] * 100 +
                                         (ulong)e.MSG[4];
                        
                            byte productId = e.MSG[5];
                            byte bridgeId = e.MSG[6];
                            
                            lock (responseLock)
                            {
                                // Check if we already found this device (by CAN ID AND serial number)
                                // This allows detecting multiple devices with same CAN ID but different serials
                                if (!bootloaderDevs.Any(d => d.CanId == e.ID_Unit && d.SerialNumber == serial))
                                {
                                    var device = new BootloaderDevice
                                    {
                                        CanId = e.ID_Unit,
                                        DeviceId = productId,
                                        BootloaderVersion = 0, // Not available from heartbeat
                                        SerialNumber = serial,
                                        CurrentBridgeId = bridgeId
                                    };
                                    
                                    bootloaderDevs.Add(device);
                                    
                                    // Check if multiple devices share same CAN ID
                                    int sameCanIdCount = bootloaderDevs.Count(d => d.CanId == e.ID_Unit);
                                    if (sameCanIdCount > 1)
                                    {
                                        ConsoleHelper.WriteWarning($"Multiple devices detected on CAN ID {e.ID_Unit}! ({sameCanIdCount} devices)");
                                    }
                                    
                                    // Send acknowledgment (status 99)
                                    byte[] response = new byte[8];
                                    Array.Copy(e.MSG, response, 8);
                                    response[7] = 99;
                                    port.WriteCAN(new byte[] { 0x19, 0xFF, 0x20, e.ID_Unit }, response);
                                }
                            }
                        }
                    }
                };

                if (OperatorSettings.IsAtLeast(OperatorMode.Advanced))
                    ConsoleHelper.WriteInfo("Listening for bootloader heartbeats...");

                    // Just listen for heartbeat messages (devices in bootloader mode broadcast continuously)
                    await Task.Delay(2000); // Listen for 2 seconds

                    port.Disconnect();
                }
            }
            catch
            {
                // Scan errors are silently ignored
            }

            return bootloaderDevs;
        }

        private void DisplayBootloaderDevices(List<BootloaderDevice> devices)
        {
            Console.WriteLine();
            foreach (var device in devices.OrderBy(d => d.CanId))
            {
                Console.ForegroundColor = ConsoleColor.Yellow;
                Console.WriteLine($"  \u25B2 CAN ID {device.CanId}: IN BOOTLOADER MODE");
                Console.ForegroundColor = ConsoleColor.Gray;
                
                string serialDisplay = device.SerialNumber == 25757575755 
                    ? "NO SERIAL SET" 
                    : FormatSerialNumber(device.SerialNumber);
                
                Console.WriteLine($"    Serial Number: {serialDisplay}");
                Console.WriteLine($"    Product ID: {device.DeviceId}");
                Console.WriteLine($"    Current Bridge: {device.CurrentBridgeId}");
                Console.ResetColor();
                Console.WriteLine();
            }
        }

        private string FormatSerialNumber(ulong serial)
        {
            string serialStr = serial.ToString().PadLeft(10, '0');
            return $"{serialStr.Substring(0, 6)}-{serialStr.Substring(6, 4)}";
        }

        /// <summary>
        /// Verifies that no devices are currently in bootloader mode.
        /// This is critical between batch operations to ensure previous device has exited bootloader.
        /// </summary>
        private async Task<bool> VerifyNoBootloaderDevicesAsync(string comPortName)
        {
            // Quick scan for any bootloader devices
            var bootloaderCheck = await ScanForBootloaderDevicesAsync(comPortName);
            
            if (bootloaderCheck != null && bootloaderCheck.Count > 0)
            {
                Console.WriteLine();
                ConsoleHelper.WriteWarning("Previous device still in bootloader mode! Attempting to exit...");
                
                // Try to exit bootloader on all detected devices
                try
                {
                    using (var port = new CANPort())
                    {
                        if (port.Connect(comPortName))
                        {
                            foreach (var device in bootloaderCheck)
                            {
                                byte[] serialBytes = SerialToByteArray(device.SerialNumber);
                                byte[] exitId = { 0x19, 0xFF, 0x20, device.CanId };
                                byte[] exitMsg = new byte[8];
                                Array.Copy(serialBytes, 0, exitMsg, 0, 5);
                                exitMsg[5] = device.DeviceId;
                                exitMsg[6] = 0x00;
                                exitMsg[7] = 0x04; // Exit bootloader
                                
                                // Send exit command multiple times
                                for (int i = 0; i < 3; i++)
                                {
                                    port.WriteCAN(exitId, exitMsg);
                                    await Task.Delay(100);
                                }
                            }
                            
                            port.Disconnect();
                        }
                    }
                    
                    // Wait for devices to restart
                    await Task.Delay(1000);
                    
                    // Verify again
                    bootloaderCheck = await ScanForBootloaderDevicesAsync(comPortName);
                    if (bootloaderCheck != null && bootloaderCheck.Count > 0)
                    {
                        ConsoleHelper.WriteWarning($"{bootloaderCheck.Count} device(s) still in bootloader. They may interfere with next update.");
                        await Task.Delay(500);
                        return false;
                    }
                    
                    ConsoleHelper.WriteSuccess("Devices exited bootloader successfully.");
                }
                catch (Exception ex)
                {
                    ConsoleHelper.WriteWarning($"Error during bootloader exit verification: {ex.Message}");
                }
            }
            
            return true;
        }

        private async Task<bool> HandleBootloaderDevicesAsync(string comPortName)
        {
            ConsoleHelper.WriteWarning("Detected devices in bootloader mode!");
            DisplayBootloaderDevices(bootloaderDevices);
            Console.WriteLine();
            
            // Check for CAN ID conflicts (multiple devices on same CAN ID)
            var canIdGroups = bootloaderDevices.GroupBy(d => d.CanId).Where(g => g.Count() > 1).ToList();
            if (canIdGroups.Any())
            {
                // Automatically resolve CAN ID conflicts
                bool resolved = await AutoResolveCanIdConflictsAsync(comPortName, canIdGroups);
                if (resolved)
                {
                    // Rescan after resolving conflicts
                    Console.WriteLine();
                    bootloaderDevices = await ScanForBootloaderDevicesAsync(comPortName);
                    if (bootloaderDevices?.Count > 0)
                    {
                        // Show menu again with resolved devices
                        return await HandleBootloaderDevicesAsync(comPortName);
                    }
                    return true;
                }
                // User cancelled - return to main menu
                return false;
            }

            // Keep scan flow simple: always try to exit bootloader first,
            // then automatically continue into recovery if the device stays stuck.
            ConsoleHelper.WriteInfo("Attempting to exit bootloader mode automatically...");
            bool success = await ExitBootloaderModeAsync(comPortName);
            if (success)
            {
                bootloaderDevices = null;
                return true;
            }

            Console.WriteLine();
            ConsoleHelper.WriteWarning("Device is still in bootloader mode. Starting automatic recovery...");
            await UpdateBootloaderDevicesAsync();

            Console.WriteLine();
            bootloaderDevices = await ScanForBootloaderDevicesAsync(comPortName);
            if (bootloaderDevices?.Count > 0)
            {
                return await HandleBootloaderDevicesAsync(comPortName);
            }

            return true;
        }

        private async Task<bool> ExitBootloaderModeAsync(string comPortName)
        {
            ConsoleHelper.WriteInfo("Sending exit bootloader command to all devices...");
            
            int originalDeviceCount = bootloaderDevices.Count;
            
            try
            {
                using (var port = new CANPort())
                {
                    if (!port.Connect(comPortName))
                    {
                        ConsoleHelper.WriteError("Failed to connect to COM port.");
                        return false;
                    }

                    // Send exit bootloader command to each device
                    foreach (var device in bootloaderDevices)
                    {
                        // Command to exit bootloader mode (0x19FF24, data[0] = 200)
                        byte[] exitCommand = new byte[8];
                        exitCommand[0] = 200; // Exit bootloader command
                        
                        port.WriteCAN(new byte[] { 0x19, 0xFF, 0x24, device.CanId }, exitCommand);
                        
                        await Task.Delay(100); // Small delay between commands
                    }

                    ConsoleHelper.WriteSuccess($"Exit bootloader command sent to {originalDeviceCount} device(s).");
                    ConsoleHelper.WriteInfo("Waiting for devices to restart...");
                    
                    await Task.Delay(3000); // Wait for devices to restart
                    
                    port.Disconnect();
                }

                // Verify devices are no longer in bootloader mode
                Console.WriteLine();
                ConsoleHelper.WriteInfo("Verifying devices exited bootloader mode...");
                
                bootloaderDevices = await ScanForBootloaderDevicesAsync(comPortName);
                
                if (bootloaderDevices == null || bootloaderDevices.Count == 0)
                {
                    ConsoleHelper.WriteSuccess($"All {originalDeviceCount} device(s) successfully exited bootloader mode.");
                    return true;
                }
                else
                {
                    ConsoleHelper.WriteWarning($"{bootloaderDevices.Count} device(s) still in bootloader mode:");
                    DisplayBootloaderDevices(bootloaderDevices);
                    return false;
                }
            }
            catch (Exception ex)
            {
                ConsoleHelper.WriteError($"Error exiting bootloader mode: {ex.Message}");
                return false;
            }
        }

        /// <summary>
        /// Change the CAN ID of a device in bootloader mode.
        /// This is useful when multiple devices have the same CAN ID and cannot be addressed individually.
        /// </summary>
        private async Task ChangeBootloaderCanIdAsync(string comPortName)
        {
            Console.WriteLine();
            ConsoleHelper.WriteHeader("Change CAN ID");
            
            if (bootloaderDevices == null || bootloaderDevices.Count == 0)
            {
                ConsoleHelper.WriteError("No bootloader devices found.");
                PressAnyKey();
                return;
            }

            // Display devices
            ConsoleHelper.WriteInfo("Select device to change CAN ID:");
            Console.WriteLine();
            
            for (int i = 0; i < bootloaderDevices.Count; i++)
            {
                var device = bootloaderDevices[i];
                string serialDisplay = device.SerialNumber == 25757575755 
                    ? "NO SERIAL" 
                    : FormatSerialNumber(device.SerialNumber);
                
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.Write($"  {i + 1}. ");
                Console.ForegroundColor = ConsoleColor.White;
                Console.WriteLine($"CAN ID {device.CanId} (Serial: {serialDisplay}, Product ID: {device.DeviceId})");
                Console.ResetColor();
            }
            
            Console.WriteLine();
            Console.Write($"  Select device (1-{bootloaderDevices.Count}) or Q to cancel: ");
            
            int? selection = ReadNumericSelection(bootloaderDevices.Count);
            if (!selection.HasValue)
            {
                return;
            }
            
            var selectedDevice = bootloaderDevices[selection.Value - 1];
            
            // Check if device has no serial
            if (selectedDevice.SerialNumber == 25757575755 || selectedDevice.SerialNumber == 0)
            {
                ConsoleHelper.WriteError("Cannot change CAN ID for devices without serial number.");
                ConsoleHelper.WriteWarning("The device must have a valid serial number to be addressed individually.");
                PressAnyKey();
                return;
            }
            
            // Ask for new CAN ID
            Console.WriteLine();
            Console.Write("  Enter new CAN ID (1-254): ");
            
            string input = "";
            while (true)
            {
                var key = Console.ReadKey(true);
                
                if (key.Key == ConsoleKey.Q || key.Key == ConsoleKey.Escape)
                {
                    Console.WriteLine("Cancelled");
                    return;
                }
                
                if (key.Key == ConsoleKey.Enter && input.Length > 0)
                {
                    Console.WriteLine();
                    break;
                }
                
                if (key.Key == ConsoleKey.Backspace && input.Length > 0)
                {
                    input = input.Substring(0, input.Length - 1);
                    Console.Write("\b \b");
                    continue;
                }
                
                if (char.IsDigit(key.KeyChar) && input.Length < 3)
                {
                    input += key.KeyChar;
                    Console.Write(key.KeyChar);
                }
            }
            
            if (!byte.TryParse(input, out byte newCanId) || newCanId < 1 || newCanId > 254)
            {
                ConsoleHelper.WriteError($"Invalid CAN ID: {input}. Must be between 1 and 254.");
                PressAnyKey();
                return;
            }
            
            if (newCanId == selectedDevice.CanId)
            {
                ConsoleHelper.WriteWarning("New CAN ID is the same as current CAN ID.");
                PressAnyKey();
                return;
            }
            
            // Confirm
            Console.WriteLine();
            ConsoleHelper.WriteWarning($"Change CAN ID from {selectedDevice.CanId} to {newCanId}?");
            Console.Write("  Confirm (Y/N): ");
            
            bool confirmed = false;
            while (true)
            {
                var key = Console.ReadKey(true);
                if (key.Key == ConsoleKey.Y)
                {
                    Console.WriteLine("Y");
                    confirmed = true;
                    break;
                }
                else if (key.Key == ConsoleKey.N || key.Key == ConsoleKey.Escape)
                {
                    Console.WriteLine("N");
                    break;
                }
            }
            
            if (!confirmed)
            {
                ConsoleHelper.WriteInfo("Cancelled.");
                return;
            }
            
            // Send the change CAN ID command
            try
            {
                using (var port = new CANPort())
                {
                    if (!port.Connect(comPortName))
                    {
                        ConsoleHelper.WriteError("Failed to connect to COM port.");
                        PressAnyKey();
                        return;
                    }
                    
                    // Convert serial number to bytes
                    byte[] serialBytes = SerialToByteArray(selectedDevice.SerialNumber);
                    
                    // Build the Set CAN ID command
                    // Protocol: Send to the NEW CAN ID, with serial, product ID, bridge=0, command=0x00
                    byte[] setCanId = { 0x19, 0xFF, 0x20, newCanId };
                    byte[] setCanMsg = new byte[8];
                    Array.Copy(serialBytes, 0, setCanMsg, 0, 5);
                    setCanMsg[5] = selectedDevice.DeviceId;  // Product ID
                    setCanMsg[6] = 0x00;                      // Bridge ID (not used)
                    setCanMsg[7] = 0x00;                      // Command: 0x00 = Set CAN ID
                    
                    ConsoleHelper.WriteInfo($"Sending CAN ID change command...");
                    
                    // Send the command multiple times to ensure it's received
                    for (int i = 0; i < 3; i++)
                    {
                        port.WriteCAN(setCanId, setCanMsg);
                        await Task.Delay(100);
                    }
                    
                    port.Disconnect();
                }
                
                // Wait a moment for the device to update
                await Task.Delay(500);
                
                ConsoleHelper.WriteSuccess($"CAN ID change command sent.");
                ConsoleHelper.WriteInfo($"Device should now respond on CAN ID {newCanId}.");
                Console.WriteLine();
                ConsoleHelper.WriteInfo("Rescanning for bootloader devices...");
            }
            catch (Exception ex)
            {
                ConsoleHelper.WriteError($"Error changing CAN ID: {ex.Message}");
                PressAnyKey();
            }
        }

        /// <summary>
        /// Automatically resolve CAN ID conflicts by assigning unique temporary CAN IDs to conflicting devices.
        /// </summary>
        private async Task<bool> AutoResolveCanIdConflictsAsync(string comPortName, List<IGrouping<byte, BootloaderDevice>> conflictGroups)
        {
            Console.WriteLine();
            ConsoleHelper.WriteError("CAN ID CONFLICT DETECTED!");
            Console.WriteLine();
            
            foreach (var group in conflictGroups)
            {
                ConsoleHelper.WriteWarning($"  {group.Count()} devices share CAN ID {group.Key}:");
                foreach (var device in group)
                {
                    string serialDisplay = device.SerialNumber == 25757575755 
                        ? "NO SERIAL" 
                        : FormatSerialNumber(device.SerialNumber);
                    Console.WriteLine($"    - Serial: {serialDisplay}");
                }
            }
            
            Console.WriteLine();
            ConsoleHelper.WriteInfo("Multiple devices cannot share the same CAN ID.");
            ConsoleHelper.WriteInfo("The program will automatically assign temporary unique CAN IDs to resolve this.");
            Console.WriteLine();
            Console.Write("  Proceed with automatic CAN ID reassignment? (Y/N): ");
            
            bool confirmed = false;
            while (true)
            {
                var key = Console.ReadKey(true);
                if (key.Key == ConsoleKey.Y)
                {
                    Console.WriteLine("Y");
                    confirmed = true;
                    break;
                }
                else if (key.Key == ConsoleKey.N || key.Key == ConsoleKey.Escape)
                {
                    Console.WriteLine("N");
                    break;
                }
            }
            
            if (!confirmed)
            {
                ConsoleHelper.WriteInfo("Cancelled.");
                return false;
            }
            
            Console.WriteLine();
            
            // Find available CAN IDs for reassignment (241-254 are typically unused)
            byte nextAvailableCanId = 241;
            var usedCanIds = new HashSet<byte>(bootloaderDevices.Select(d => d.CanId));
            
            try
            {
                using (var port = new CANPort())
                {
                    if (!port.Connect(comPortName))
                    {
                        ConsoleHelper.WriteError("Failed to connect to COM port.");
                        return false;
                    }
                    
                    foreach (var group in conflictGroups)
                    {
                        // Keep the first device on its original CAN ID, reassign the rest
                        var devicesToReassign = group.Skip(1).ToList();
                        
                        foreach (var device in devicesToReassign)
                        {
                            // Skip devices without serial (cannot be addressed individually)
                            if (device.SerialNumber == 25757575755 || device.SerialNumber == 0)
                            {
                                ConsoleHelper.WriteWarning($"Cannot reassign device without serial number on CAN ID {device.CanId}");
                                continue;
                            }
                            
                            // Find next available CAN ID
                            while (usedCanIds.Contains(nextAvailableCanId) && nextAvailableCanId < 254)
                            {
                                nextAvailableCanId++;
                            }
                            
                            if (nextAvailableCanId >= 254)
                            {
                                ConsoleHelper.WriteError("No available CAN IDs for reassignment!");
                                port.Disconnect();
                                return false;
                            }
                            
                            string serialDisplay = FormatSerialNumber(device.SerialNumber);
                            ConsoleHelper.WriteInfo($"Reassigning {serialDisplay} from CAN ID {device.CanId} to {nextAvailableCanId}...");
                            
                            // Convert serial number to bytes
                            byte[] serialBytes = SerialToByteArray(device.SerialNumber);
                            
                            // Build the Set CAN ID command
                            byte[] setCanId = { 0x19, 0xFF, 0x20, nextAvailableCanId };
                            byte[] setCanMsg = new byte[8];
                            Array.Copy(serialBytes, 0, setCanMsg, 0, 5);
                            setCanMsg[5] = device.DeviceId;  // Product ID
                            setCanMsg[6] = 0x00;              // Bridge ID (not used)
                            setCanMsg[7] = 0x00;              // Command: 0x00 = Set CAN ID
                            
                            // Send the command multiple times to ensure it's received
                            for (int i = 0; i < 3; i++)
                            {
                                port.WriteCAN(setCanId, setCanMsg);
                                await Task.Delay(100);
                            }
                            
                            ConsoleHelper.WriteSuccess($"  -> {serialDisplay} now on CAN ID {nextAvailableCanId}");
                            
                            // Mark this CAN ID as used
                            usedCanIds.Add(nextAvailableCanId);
                            nextAvailableCanId++;
                        }
                    }
                    
                    port.Disconnect();
                }
                
                // Wait for devices to update
                await Task.Delay(500);
                
                Console.WriteLine();
                ConsoleHelper.WriteSuccess("CAN ID conflicts resolved!");
                ConsoleHelper.WriteInfo("Devices can now be updated individually.");
                
                return true;
            }
            catch (Exception ex)
            {
                ConsoleHelper.WriteError($"Error resolving CAN ID conflicts: {ex.Message}");
                return false;
            }
        }

        private byte[] SerialToByteArray(ulong serial)
        {
            // Convert serial number to 5 bytes (BCD-like format)
            // Serial format: NNNNNN-MMMM -> stored as 10 digits
            string serialStr = serial.ToString().PadLeft(10, '0');
            
            byte[] bytes = new byte[5];
            bytes[0] = byte.Parse(serialStr.Substring(0, 2));
            bytes[1] = byte.Parse(serialStr.Substring(2, 2));
            bytes[2] = byte.Parse(serialStr.Substring(4, 2));
            bytes[3] = byte.Parse(serialStr.Substring(6, 2));
            bytes[4] = byte.Parse(serialStr.Substring(8, 2));
            
            return bytes;
        }

        private async Task ScanDevicesAsync()
        {
            // Force a fresh scan
            discoveredDevices = null;
            bootloaderDevices = null;
            var devices = await EnsureDevicesScannedAsync();

            if ((devices == null || devices.Count == 0) && (bootloaderDevices == null || bootloaderDevices.Count == 0))
            {
                PressAnyKey();
                return;
            }

            PressAnyKey();
        }

        private async Task UpdateFirmwareAsync()
        {
            ConsoleHelper.WriteHeader("Firmware Update");

            // FORCE A FRESH SCAN: Always scan for devices
            discoveredDevices = null;
            bootloaderDevices = null;
            var devices = await EnsureDevicesScannedAsync();

            // Check if we found devices in bootloader mode
            if ((devices == null || devices.Count == 0) && bootloaderDevices?.Count > 0)
            {
                Console.WriteLine();
                ConsoleHelper.WriteWarning("Devices detected in bootloader mode only.");
                ConsoleHelper.WriteInfo("Recovery uses recorded device history and compatible cached firmware only.");
                Console.WriteLine();
                
                // Ask if user wants to proceed with bootloader-mode recovery
                Console.Write("  Continue with automatic recovery? (Y/N): ");
                
                bool proceed = false;
                while (true)
                {
                    var key = Console.ReadKey(true);
                    if (key.Key == ConsoleKey.Y)
                    {
                        Console.WriteLine("Y");
                        proceed = true;
                        break;
                    }
                    else if (key.Key == ConsoleKey.N || key.Key == ConsoleKey.Escape)
                    {
                        Console.WriteLine("N");
                        proceed = false;
                        break;
                    }
                }
                
                if (!proceed)
                {
                    PressAnyKey();
                    return;
                }
                
                await UpdateBootloaderDevicesAsync();
                return;
            }

            if (devices == null || devices.Count == 0)
            {
                PressAnyKey();
                return;
            }

            Console.WriteLine();

            // Simple Mode: Auto-update all devices with newer firmware
            if (OperatorSettings.CurrentMode == OperatorMode.Simple)
            {
                // Get list of updates needed (only newer versions)
                var updatesNeeded = GetUpdatesNeeded(devices);

                if (updatesNeeded.Count == 0)
                {
                    ConsoleHelper.WriteInfo("No firmware updates available.");
                    PressAnyKey();
                    return;
                }

                await UpdateAllFirmwareAsync(updatesNeeded);
            }
            // Advanced/Debug Mode: Manual selection with all firmware options
            else
            {
                await UpdateFirmwareManualAsync(devices);
            }
        }

        // Helper method for reading multi-digit numeric input
        private int? ReadNumericSelection(int maxValue, bool allowCancel = true)
        {
            string input = "";
            
            while (true)
            {
                var key = Console.ReadKey(true);
                
                // Handle cancel
                if (allowCancel && (key.Key == ConsoleKey.Q || key.Key == ConsoleKey.Escape))
                {
                    Console.WriteLine("Q");
                    return null;
                }
                
                // Handle Enter - submit current input
                if (key.Key == ConsoleKey.Enter)
                {
                    if (string.IsNullOrEmpty(input))
                    {
                        continue;
                    }
                    
                    if (int.TryParse(input, out int selection))
                    {
                        if (selection >= 1 && selection <= maxValue)
                        {
                            Console.WriteLine(); // Move to next line
                            return selection;
                        }
                    }
                    
                    // Clear the invalid input and let user try again
                    for (int i = 0; i < input.Length; i++)
                        Console.Write("\b \b");
                    input = "";
                    continue;
                }
                
                // Handle Backspace
                if (key.Key == ConsoleKey.Backspace)
                {
                    if (input.Length > 0)
                    {
                        input = input.Substring(0, input.Length - 1);
                        Console.Write("\b \b");
                    }
                    continue;
                }
                
                // Handle numeric input (digits only)
                if (char.IsDigit(key.KeyChar))
                {
                    // Prevent input that would be too large
                    string testInput = input + key.KeyChar;
                    if (int.TryParse(testInput, out int testValue) && testValue <= maxValue * 10)
                    {
                        input += key.KeyChar;
                        Console.Write(key.KeyChar);
                    }
                    continue;
                }
            }
        }

        private async Task UpdateBootloaderDevicesAsync()
        {
            ConsoleHelper.WriteHeader("Update Device in Bootloader Mode");
            Console.WriteLine();
            bool isSimpleMode = OperatorSettings.CurrentMode == OperatorMode.Simple;
            
            // Display available bootloader devices
            ConsoleHelper.WriteInfo("Devices in bootloader mode:");
            Console.WriteLine();
            
            for (int i = 0; i < bootloaderDevices.Count; i++)
            {
                var device = bootloaderDevices[i];
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.Write($"  {i + 1}. ");
                Console.ForegroundColor = ConsoleColor.Yellow;
                Console.Write($"CAN ID {device.CanId}");
                Console.ResetColor();
                
                string serialDisplay = device.SerialNumber == 25757575755 
                    ? "NO SERIAL SET" 
                    : FormatSerialNumber(device.SerialNumber);
                
                Console.WriteLine($" (Serial: {serialDisplay}, Product ID: {device.DeviceId}, Bridge: {device.CurrentBridgeId})");
            }
            
            Console.WriteLine();
            Console.Write($"  Select device (1-{bootloaderDevices.Count}) or Q to cancel: ");
            
            int? deviceSelection = ReadNumericSelection(bootloaderDevices.Count);
            if (!deviceSelection.HasValue)
            {
                PressAnyKey();
                return;
            }
            
            BootloaderDevice selectedDevice = bootloaderDevices[deviceSelection.Value - 1];
            
            // Try automatic firmware lookup based on serial number
            FirmwareInfo autoSelectedFirmware = null;
            FirmwareInfo cachedExistingFirmware = null;
            FirmwareInfo cachedLatestFirmware = null;
            string lastKnownPartNumber = null;
            string lastKnownVersion = null;
            byte targetBridgeId = selectedDevice.CurrentBridgeId;
            
            if (logger != null && selectedDevice.SerialNumber != 25757575755)
            {
                string serialString = selectedDevice.SerialNumber.ToString().PadLeft(10, '0');
                Console.WriteLine();
                ConsoleHelper.WriteInfo($"Looking up device history for S/N: {serialString}...");
                
                var lastKnown = await logger.GetLastKnownFirmwareAsync(serialString);
                if (lastKnown.HasValue)
                {
                    string partNumber = lastKnown.Value.PartNumber;
                    string bridgeIdStr = lastKnown.Value.BridgeId;
                    string lastVersion = lastKnown.Value.Version;
                    lastKnownPartNumber = partNumber;
                    lastKnownVersion = lastVersion;
                    
                    ConsoleHelper.WriteSuccess($"Found device history: {partNumber} (Bridge {bridgeIdStr})");
                    if (!string.IsNullOrEmpty(lastVersion))
                    {
                        Console.WriteLine($"  Last known version: v{lastVersion}");
                    }
                    
                    // Try to find matching firmware in cache
                    if (byte.TryParse(bridgeIdStr, out byte dbBridgeId))
                    {
                        // Extract major.minor version from last known version (format: NN.NN.XX)
                        string lastMajorMinor = null;
                        if (!string.IsNullOrEmpty(lastVersion))
                        {
                            var versionParts = lastVersion.Split('.');
                            if (versionParts.Length >= 2)
                            {
                                lastMajorMinor = $"{versionParts[0]}.{versionParts[1]}";
                            }
                        }
                        
                        // Get firmware with matching part number, bridge ID, AND same major.minor version
                        var compatibleFirmware = cache.GetAllFirmware()
                            .Where(f => f.PartNumber == partNumber && f.BridgeId == dbBridgeId)
                            .Where(f => {
                                if (isSimpleMode && f.IsPrototype)
                                    return false;

                                if (lastMajorMinor == null)
                                    return true;

                                var fwParts = f.VersionString.Split('.');
                                if (fwParts.Length >= 2)
                                {
                                    string fwMajorMinor = $"{fwParts[0]}.{fwParts[1]}";
                                    return fwMajorMinor == lastMajorMinor;
                                }
                                return false;
                            })
                            .OrderByDescending(f => f.Version)
                            .ToList();

                        cachedLatestFirmware = compatibleFirmware.FirstOrDefault();
                        cachedExistingFirmware = compatibleFirmware
                            .FirstOrDefault(f => !string.IsNullOrEmpty(lastVersion) && f.VersionString == lastVersion);
                        
                        if (cachedLatestFirmware != null)
                        {
                            autoSelectedFirmware = cachedExistingFirmware ?? cachedLatestFirmware;
                            targetBridgeId = dbBridgeId;
                            Console.WriteLine();
                            ConsoleHelper.WriteSuccess($"Auto-selected firmware: {autoSelectedFirmware.PartNumber} v{autoSelectedFirmware.VersionString}");
                        }
                        else if (lastMajorMinor != null)
                        {
                            Console.WriteLine();
                            ConsoleHelper.WriteWarning($"No firmware with version {lastMajorMinor}.XX found in cache.");
                        }
                    }
                }
                else
                {
                    ConsoleHelper.WriteWarning("No device history found.");
                }
            }

            if (isSimpleMode)
            {
                // In simple mode, keep recovery mostly automatic and only ask for one decision.
                if (cachedLatestFirmware != null)
                {
                    bool hasDistinctExistingOption = cachedExistingFirmware != null &&
                        cachedExistingFirmware.VersionString != cachedLatestFirmware.VersionString;

                    Console.WriteLine();
                    ConsoleHelper.WriteWarning("Automatic recovery: device information is unavailable");
                    Console.WriteLine();
                    Console.WriteLine($"  Device: CAN ID {selectedDevice.CanId} (Serial: {FormatSerialNumber(selectedDevice.SerialNumber)})");
                    Console.WriteLine($"  Product: {lastKnownPartNumber}");
                    Console.WriteLine($"  Bridge:  {cachedLatestFirmware.BridgeId}");
                    Console.WriteLine();

                    if (hasDistinctExistingOption)
                    {
                        Console.WriteLine("  1. Re-install existing firmware");
                        Console.WriteLine($"     v{cachedExistingFirmware.VersionString}");
                        Console.WriteLine("  2. Try update to latest firmware again");
                        Console.WriteLine($"     v{cachedLatestFirmware.VersionString}");
                        Console.WriteLine();
                        Console.Write("  Choose 1, 2 or Q to cancel: ");

                        while (true)
                        {
                            var key = Console.ReadKey(true);
                            if (key.KeyChar == '1')
                            {
                                Console.WriteLine("1");
                                await PerformBootloaderModeUpdate(selectedDevice.CanId, selectedDevice.SerialNumber, cachedExistingFirmware.BridgeId, cachedExistingFirmware);
                                return;
                            }

                            if (key.KeyChar == '2')
                            {
                                Console.WriteLine("2");
                                await PerformBootloaderModeUpdate(selectedDevice.CanId, selectedDevice.SerialNumber, cachedLatestFirmware.BridgeId, cachedLatestFirmware);
                                return;
                            }

                            if (key.Key == ConsoleKey.Q || key.Key == ConsoleKey.Escape)
                            {
                                Console.WriteLine("Q");
                                ConsoleHelper.WriteInfo("Recovery cancelled.");
                                PressAnyKey();
                                return;
                            }
                        }
                    }

                    if (cachedExistingFirmware != null)
                    {
                        ConsoleHelper.WriteInfo($"Existing firmware found in cache: v{cachedExistingFirmware.VersionString}");
                        if (cachedExistingFirmware.VersionString == cachedLatestFirmware.VersionString)
                        {
                            ConsoleHelper.WriteInfo("Existing firmware is already the latest available.");
                        }
                    }
                    else
                    {
                        ConsoleHelper.WriteWarning("Existing firmware is not available in cache.");
                    }

                    Console.WriteLine();
                    Console.Write($"  Try update to latest firmware again (v{cachedLatestFirmware.VersionString})? (Y/N): ");
                    while (true)
                    {
                        var key = Console.ReadKey(true);
                        if (key.Key == ConsoleKey.Y)
                        {
                            Console.WriteLine("Y");
                            await PerformBootloaderModeUpdate(selectedDevice.CanId, selectedDevice.SerialNumber, cachedLatestFirmware.BridgeId, cachedLatestFirmware);
                            return;
                        }

                        if (key.Key == ConsoleKey.N || key.Key == ConsoleKey.Escape)
                        {
                            Console.WriteLine("N");
                            ConsoleHelper.WriteInfo("Recovery cancelled.");
                            PressAnyKey();
                            return;
                        }
                    }
                }

                Console.WriteLine();
                if (string.IsNullOrEmpty(lastKnownPartNumber))
                {
                    ConsoleHelper.WriteWarning("Automatic recovery is not available because no device history was found.");
                }
                else
                {
                    ConsoleHelper.WriteWarning($"Automatic recovery is not available because compatible firmware for {lastKnownPartNumber} could not be found in cache.");
                    if (!string.IsNullOrEmpty(lastKnownVersion))
                    {
                        ConsoleHelper.WriteInfo($"Last known version was v{lastKnownVersion}.");
                    }
                }

                ConsoleHelper.WriteInfo("Bootloader recovery is restricted to automatic, history-based recovery only.");
                PressAnyKey();
                return;
            }

            if (autoSelectedFirmware == null)
            {
                Console.WriteLine();
                if (string.IsNullOrEmpty(lastKnownPartNumber))
                {
                    ConsoleHelper.WriteWarning("Automatic recovery is not available because no device history was found.");
                }
                else
                {
                    ConsoleHelper.WriteWarning($"Automatic recovery is not available because compatible firmware for {lastKnownPartNumber} could not be found in cache.");
                    if (!string.IsNullOrEmpty(lastKnownVersion))
                    {
                        ConsoleHelper.WriteInfo($"Last known version was v{lastKnownVersion}.");
                    }
                }

                ConsoleHelper.WriteInfo("Bootloader recovery is restricted to automatic, history-based recovery only.");
                PressAnyKey();
                return;
            }

            Console.WriteLine();
            ConsoleHelper.WriteWarning("Automatic recovery: device information is unavailable");
            Console.WriteLine();
            Console.WriteLine($"  Device:   CAN ID {selectedDevice.CanId}");
            Console.WriteLine($"  Serial:   {FormatSerialNumber(selectedDevice.SerialNumber)}");
            Console.WriteLine($"  Firmware: {autoSelectedFirmware.PartNumber} - Bridge {autoSelectedFirmware.BridgeId} - v{autoSelectedFirmware.VersionString}");
            Console.WriteLine();
            Console.Write("  Continue with automatic recovery? (Y/N): ");

            while (true)
            {
                var key = Console.ReadKey(true);
                if (key.Key == ConsoleKey.Y)
                {
                    Console.WriteLine("Y");
                    await PerformBootloaderModeUpdate(selectedDevice.CanId, selectedDevice.SerialNumber, autoSelectedFirmware.BridgeId, autoSelectedFirmware);
                    return;
                }
                else if (key.Key == ConsoleKey.N || key.Key == ConsoleKey.Escape)
                {
                    Console.WriteLine("N");
                    ConsoleHelper.WriteInfo("Update cancelled.");
                    PressAnyKey();
                    return;
                }
            }
        }

        private async Task PerformBootloaderModeUpdate(byte canId, ulong bootloaderSerial, byte bridgeId, FirmwareInfo firmware)
        {
            string tempHexFilePath = null;

            try
            {
                Console.WriteLine();
                ConsoleHelper.WriteInfo("Loading firmware from encrypted cache...");
                byte[] decryptedData = cache.LoadFirmwareData(firmware);

                tempHexFilePath = Path.Combine(Path.GetTempPath(), firmware.FileName);
                await File.WriteAllBytesAsync(tempHexFilePath, decryptedData);

                ConsoleHelper.WriteSuccess($"Firmware loaded: {firmware.FileName}");

                string comPort = CANPort.FindPortByVidPid(
                    "0483", "5740",  // CP - IOT Modem
                    "04D8", "000A"   // CP - CAN Modem
                );

                if (string.IsNullOrEmpty(comPort))
                {
                    ConsoleHelper.WriteError("No COM port found.");
                    return;
                }

                Console.WriteLine();
                var uploader = new FirmwareUploader();

                // Format serial number - use the actual bootloader serial for proper device targeting
                // This is critical when multiple devices share the same CAN ID
                string serialString = bootloaderSerial == 25757575755 
                    ? "BOOTLOADER"  // No serial - use special marker
                    : bootloaderSerial.ToString().PadLeft(10, '0');
                
                bool success = await uploader.UploadFirmwareAsync(
                    comPort,
                    canId,
                    serialString,
                    bridgeId,
                    tempHexFilePath,
                    false,
                    alreadyInBootloader: true,
                    bootloaderLogger: logger,
                    userIdentification: Environment.UserName,
                    originalVersion: "Unknown",
                    targetVersion: firmware?.VersionString ?? "Unknown",
                    partNumber: firmware?.PartNumber
                );

                if (!success)
                {
                    ConsoleHelper.WriteError("Firmware update failed.");
                }
            }
            catch (Exception ex)
            {
                ConsoleHelper.WriteError($"Failed to update firmware: {ex.Message}");
            }
            finally
            {
                CleanupTempFile(tempHexFilePath);
                PressAnyKey();
            }
        }

        private List<(CANDevice device, byte bridgeId, FirmwareInfo firmware)> GetUpdatesNeeded(List<CANDevice> devices)
        {
            var updates = new List<(CANDevice, byte, FirmwareInfo)>();
            bool isSimpleMode = OperatorSettings.CurrentMode == OperatorMode.Simple;

            foreach (var device in devices.Where(d => !string.IsNullOrEmpty(d.PartNumber)))
            {
                // Check all possible bridges (1-4)
                for (byte bridgeId = 1; bridgeId <= 4; bridgeId++)
                {
                    int currentFirmware = device.BridgeFirmwareVersions.GetValueOrDefault(bridgeId, 0);
                    if (currentFirmware == 0) continue; // Skip bridges not present

                    // Get all firmware versions from cache
                    var allVersions = cache.GetAllFirmwareVersions(device.PartNumber, bridgeId);
                    
                    if (allVersions.Count > 0)
                    {
                        // Filter to only compatible versions (same hardware version NN.NN)
                        int currentHardwareVersion = currentFirmware / 100;  // Remove last 2 digits (patch)
                        
                        var compatibleVersions = allVersions
                            .Where(f => {
                                // In Simple mode, filter out prototype firmware
                                if (isSimpleMode && f.IsPrototype)
                                    return false;
                                    
                                int fwInt = CANDevice.ConvertFirmwareToInt(f.VersionString);
                                int fwHardwareVersion = fwInt / 100;
                                return fwHardwareVersion == currentHardwareVersion;
                            })
                            .ToList();
                        
                        // Get the latest compatible version (highest YY)
                        var latestFirmware = compatibleVersions
                            .OrderByDescending(f => f.Version)
                            .FirstOrDefault();
                        
                        if (latestFirmware != null)
                        {
                            int latestFirmwareInt = CANDevice.ConvertFirmwareToInt(latestFirmware.VersionString);

                            // Only include if newer version available
                            if (latestFirmwareInt > currentFirmware)
                            {
                                updates.Add((device, bridgeId, latestFirmware));
                            }
                        }
                    }
                }
            }

            return updates;
        }

        /// <summary>
        /// Gets a list of firmware reloads available - finds exact version matches in cache.
        /// </summary>
        private List<(CANDevice device, byte bridgeId, FirmwareInfo firmware)> GetReloadsAvailable(List<CANDevice> devices)
        {
            var reloads = new List<(CANDevice, byte, FirmwareInfo)>();
            bool isSimpleMode = OperatorSettings.CurrentMode == OperatorMode.Simple;

            foreach (var device in devices.Where(d => !string.IsNullOrEmpty(d.PartNumber)))
            {
                // Check all possible bridges (1-4)
                for (byte bridgeId = 1; bridgeId <= 4; bridgeId++)
                {
                    int currentFirmware = device.BridgeFirmwareVersions.GetValueOrDefault(bridgeId, 0);
                    if (currentFirmware == 0) continue; // Skip bridges not present

                    string currentVersionString = CANDevice.ConvertFirmwareToString(currentFirmware);

                    // Get all firmware versions from cache
                    var allVersions = cache.GetAllFirmwareVersions(device.PartNumber, bridgeId);
                    
                    // Find exact version match (in Simple mode, skip prototype firmware)
                    var exactMatch = allVersions.FirstOrDefault(f => 
                        f.VersionString == currentVersionString && 
                        (!isSimpleMode || !f.IsPrototype));
                    
                    if (exactMatch != null)
                    {
                        reloads.Add((device, bridgeId, exactMatch));
                    }
                }
            }

            return reloads;
        }

        /// <summary>
        /// Reload current firmware on devices (re-flash same version).
        /// </summary>
        private async Task ReloadCurrentFirmwareAsync()
        {
            ConsoleHelper.WriteHeader("Reload Current Firmware");

            // FORCE A FRESH SCAN: Always scan for devices
            discoveredDevices = null;
            bootloaderDevices = null;
            var devices = await EnsureDevicesScannedAsync();

            if (devices == null || devices.Count == 0)
            {
                ConsoleHelper.WriteInfo("No devices found on CAN bus.");
                PressAnyKey();
                return;
            }

            // Get list of reloads available (exact version matches in cache)
            var reloadsAvailable = GetReloadsAvailable(devices);

            if (reloadsAvailable.Count == 0)
            {
                ConsoleHelper.WriteInfo("No matching firmware found in cache for current device versions.");
                ConsoleHelper.WriteWarning("Tip: Ensure firmware cache is up to date (option 3 in Advanced mode).");
                PressAnyKey();
                return;
            }

            Console.WriteLine();
            ConsoleHelper.WriteInfo($"Found {reloadsAvailable.Count} firmware reload(s) available.");
            Console.WriteLine();

            // List all reloads
            foreach (var (device, bridgeId, firmware) in reloadsAvailable)
            {
                string currentVersion = firmware.VersionString;
                
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.Write($"  * CAN ID {device.CanId} - {device.PartNumber} - Bridge {bridgeId}");
                Console.ResetColor();
                Console.Write($": v{currentVersion} -> ");
                Console.ForegroundColor = ConsoleColor.Yellow;
                Console.WriteLine($"v{currentVersion} (reload)");
                Console.ResetColor();
            }

            Console.WriteLine();
            ConsoleHelper.WriteWarning("Reload all listed firmware?");
            Console.Write("  Continue? (Y/N): ");

            // Confirm batch reload
            bool confirmed = false;
            while (true)
            {
                var key = Console.ReadKey(true);

                if (key.Key == ConsoleKey.Y)
                {
                    Console.WriteLine("Y");
                    confirmed = true;
                    break;
                }
                else if (key.Key == ConsoleKey.N || key.Key == ConsoleKey.Escape)
                {
                    Console.WriteLine("N");
                    confirmed = false;
                    break;
                }
            }

            if (!confirmed)
            {
                ConsoleHelper.WriteInfo("Reload cancelled.");
                PressAnyKey();
                return;
            }

            // Use the same batch update logic
            await PerformBatchReloadAsync(reloadsAvailable);
        }

        /// <summary>
        /// Performs batch reload of firmware using the optimized batch update logic.
        /// </summary>
        private async Task PerformBatchReloadAsync(List<(CANDevice device, byte bridgeId, FirmwareInfo firmware)> reloads)
        {
            Console.WriteLine();

            // Group reloads by CAN ID to optimize batch operations on same device
            var groupedReloads = reloads
                .Select((reload, index) => (reload, index))
                .GroupBy(x => x.reload.device.CanId)
                .OrderBy(g => g.Min(x => x.index))
                .ToList();

            int successCount = 0;
            int failCount = 0;
            int reloadIndex = 0;

            foreach (var deviceGroup in groupedReloads)
            {
                var deviceReloads = deviceGroup.OrderBy(x => x.index).Select(x => x.reload).ToList();

                for (int j = 0; j < deviceReloads.Count; j++)
                {
                    var (device, bridgeId, firmware) = deviceReloads[j];
                    reloadIndex++;
                    
                    Console.WriteLine();
                    ConsoleHelper.WriteHeader($"Reload {reloadIndex} of {reloads.Count}");
                    Console.ForegroundColor = ConsoleColor.White;
                    Console.WriteLine($"  Device: CAN ID {device.CanId} - {device.PartNumber} (S/N: {device.SerialString})");
                    Console.WriteLine($"  Bridge: {bridgeId} ({GetBridgeName(bridgeId)})");
                    Console.Write($"  Version: v{firmware.VersionString} (reload)");
                    if (firmware.IsPrototype)
                    {
                        Console.ForegroundColor = ConsoleColor.Magenta;
                        Console.Write(" [PROTOTYPE]");
                        Console.ForegroundColor = ConsoleColor.White;
                    }
                    Console.WriteLine();
                    Console.ResetColor();

                    bool success = await PerformSingleFirmwareUpdate(device, bridgeId, firmware);
                    
                    if (success)
                    {
                        successCount++;
                        Console.WriteLine();
                    }
                    else
                    {
                        failCount++;
                        ConsoleHelper.WriteWarning("Continuing with next reload...");
                        await Task.Delay(2000);
                    }
                }

                // CRITICAL: Verify no bootloader devices before continuing to next device group
                // This prevents scenario where previous device is still in bootloader and interferes
                if (groupedReloads.IndexOf(deviceGroup) < groupedReloads.Count - 1)
                {
                    await VerifyNoBootloaderDevicesAsync(currentComPortName);
                }
            }

            // Summary
            Console.WriteLine();
            ConsoleHelper.WriteHeader("Reload Summary");
            Console.ForegroundColor = ConsoleColor.Green;
            Console.WriteLine($"  \u2713 Successful: {successCount}");
            if (failCount > 0)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($"  \u2717 Failed: {failCount}");
            }
            Console.ResetColor();

            PressAnyKey();
        }

        private async Task UpdateAllFirmwareAsync(List<(CANDevice device, byte bridgeId, FirmwareInfo firmware)> updates)
        {
            Console.WriteLine();
            ConsoleHelper.WriteInfo($"Found {updates.Count} firmware update(s) available.");
            Console.WriteLine();

            // List all updates
            foreach (var (device, bridgeId, firmware) in updates)
            {
                int currentFirmware = device.BridgeFirmwareVersions.GetValueOrDefault(bridgeId, 0);
                string currentVersion = CANDevice.ConvertFirmwareToString(currentFirmware);
                
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.Write($"  * CAN ID {device.CanId} - {device.PartNumber} - Bridge {bridgeId}");
                Console.ResetColor();
                Console.Write($": v{currentVersion} -> ");
                Console.ForegroundColor = ConsoleColor.Green;
                Console.WriteLine($"v{firmware.VersionString}");
                Console.ResetColor();
            }

            Console.WriteLine();
            ConsoleHelper.WriteWarning("Update all listed firmware?");
            Console.Write("  Continue? (Y/N): ");

            // Confirm batch update
            bool confirmed = false;
            while (true)
            {
                var key = Console.ReadKey(true);

                if (key.Key == ConsoleKey.Y)
                {
                    Console.WriteLine("Y");
                    confirmed = true;
                    break;
                }
                else if (key.Key == ConsoleKey.N || key.Key == ConsoleKey.Escape)
                {
                    Console.WriteLine("N");
                    confirmed = false;
                    break;
                }
            }

            if (!confirmed)
            {
                ConsoleHelper.WriteInfo("Update cancelled.");
                PressAnyKey();
                return;
            }

            Console.WriteLine();

            // Group updates by CAN ID to optimize batch updates on same device
            // This keeps COM port open and device in bootloader between bridge updates
            var groupedUpdates = updates
                .Select((update, index) => (update, index))
                .GroupBy(x => x.update.device.CanId)
                .OrderBy(g => g.Min(x => x.index)) // Preserve original order by first appearance
                .ToList();

            // Perform updates sequentially, with batch optimization for same CAN ID
            int successCount = 0;
            int failCount = 0;
            int updateIndex = 0;

            foreach (var deviceGroup in groupedUpdates)
            {
                var deviceUpdates = deviceGroup.OrderBy(x => x.index).Select(x => x.update).ToList();

                for (int j = 0; j < deviceUpdates.Count; j++)
                {
                    var (device, bridgeId, firmware) = deviceUpdates[j];
                    updateIndex++;
                    
                    Console.WriteLine();
                    ConsoleHelper.WriteHeader($"Update {updateIndex} of {updates.Count}");
                    Console.ForegroundColor = ConsoleColor.White;
                    Console.WriteLine($"  Device: CAN ID {device.CanId} - {device.PartNumber} (S/N: {device.SerialString})");
                    Console.WriteLine($"  Bridge: {bridgeId} ({GetBridgeName(bridgeId)})");
                    Console.Write($"  Version: v{CANDevice.ConvertFirmwareToString(device.BridgeFirmwareVersions[bridgeId])} -> v{firmware.VersionString}");
                    if (firmware.IsPrototype)
                    {
                        Console.ForegroundColor = ConsoleColor.Magenta;
                        Console.Write(" [PROTOTYPE]");
                        Console.ForegroundColor = ConsoleColor.White;
                    }
                    Console.WriteLine();
                    Console.ResetColor();

                    bool success = await PerformSingleFirmwareUpdate(device, bridgeId, firmware);
                    
                    if (success)
                    {
                        successCount++;
                        Console.WriteLine();
                    }
                    else
                    {
                        failCount++;
                        ConsoleHelper.WriteWarning("Continuing with next update...");
                        await Task.Delay(2000);
                    }
                }

                // CRITICAL: Verify no bootloader devices before continuing to next device group
                // This prevents scenario where previous device is still in bootloader and interferes
                if (groupedUpdates.IndexOf(deviceGroup) < groupedUpdates.Count - 1)
                {
                    await VerifyNoBootloaderDevicesAsync(currentComPortName);
                }
            }

            // Summary
            Console.WriteLine();
            ConsoleHelper.WriteHeader("Update Summary");
            Console.ForegroundColor = ConsoleColor.Green;
            Console.WriteLine($"  \u2713 Successful: {successCount}");
            if (failCount > 0)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($"  \u2717 Failed: {failCount}");
            }
            Console.ResetColor();

            PressAnyKey();
        }

        private async Task UpdateFirmwareManualAsync(List<CANDevice> devices)
        {
            // Build a list of devices with available firmware (including ALL cached versions for manual selection)
            var devicesWithFirmware = GetDevicesWithAvailableFirmware(devices);

            if (devicesWithFirmware.Count == 0)
            {
                ConsoleHelper.WriteInfo("No firmware available in cache for any discovered devices.");
                PressAnyKey();
                return;
            }

            // Select device and bridge
            var selection = SelectDeviceAndBridgeForUpdate(devicesWithFirmware);
            if (selection == null)
            {
                PressAnyKey();
                return;
            }

            var (selectedDevice, bridgeId, availableVersions) = selection.Value;

            // Validate serial number
            if (string.IsNullOrEmpty(selectedDevice.SerialString) || selectedDevice.SerialNumber == 0)
            {
                ConsoleHelper.WriteError("Device does not have a valid serial number. Cannot proceed with firmware update.");
                PressAnyKey();
                return;
            }

            // In Advanced/Debug mode, the user already selected a specific firmware version
            // In Simple mode, use the latest (first) version
            FirmwareInfo selectedFirmware = availableVersions[0];

            // Display current and new firmware versions
            int currentFirmware = selectedDevice.BridgeFirmwareVersions.GetValueOrDefault(bridgeId, 0);
            string currentVersion = currentFirmware > 0 ? CANDevice.ConvertFirmwareToString(currentFirmware) : "Unknown";

            Console.WriteLine();
            ConsoleHelper.WriteInfo($"Device: CAN ID {selectedDevice.CanId} - {selectedDevice.PartNumber}");
            ConsoleHelper.WriteInfo($"Bridge {bridgeId}: {GetBridgeName(bridgeId)}");
            ConsoleHelper.WriteInfo($"Serial Number: {selectedDevice.SerialString}");
            Console.WriteLine();
            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.Write($"  Current firmware: ");
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine($"v{currentVersion}");
            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.Write($"  Target firmware:  ");
            Console.ForegroundColor = ConsoleColor.Green;
            Console.WriteLine($"v{selectedFirmware.VersionString}");
            Console.ResetColor();

            // Load and perform update
            await PerformFirmwareUpdate(selectedDevice, bridgeId, selectedFirmware);
        }

        private List<(CANDevice device, byte bridgeId, List<FirmwareInfo> versions)> GetDevicesWithAvailableFirmware(List<CANDevice> devices)
        {
            var devicesWithFirmware = new List<(CANDevice, byte, List<FirmwareInfo>)>();

            foreach (var device in devices.Where(d => !string.IsNullOrEmpty(d.PartNumber)))
            {
                // Check all possible bridges (1-4)
                for (byte bridgeId = 1; bridgeId <= 4; bridgeId++)
                {
                    int currentFirmware = device.BridgeFirmwareVersions.GetValueOrDefault(bridgeId, 0);
                    if (currentFirmware == 0)
                        continue;
                    
                    // Get all available firmware versions for this part number and bridge
                    var allVersions = cache.GetAllFirmwareVersions(device.PartNumber, bridgeId)
                        .OrderByDescending(f => f.Version)
                        .ToList();

                    int currentHardwareVersion = currentFirmware / 100;
                    List<FirmwareInfo> availableVersions = allVersions
                        .Where(f => (CANDevice.ConvertFirmwareToInt(f.VersionString) / 100) == currentHardwareVersion)
                        .ToList();

                    if (availableVersions.Count > 0)
                    {
                        devicesWithFirmware.Add((device, bridgeId, availableVersions));
                    }
                }
            }

            return devicesWithFirmware;
        }

        private (CANDevice device, byte bridgeId, List<FirmwareInfo> versions)? SelectDeviceAndBridgeForUpdate(
            List<(CANDevice device, byte bridgeId, List<FirmwareInfo> versions)> devicesWithFirmware)
        {
            ConsoleHelper.WriteHeader("Select Device/Bridge to Update");

            ConsoleHelper.WriteInfo("Showing all devices with firmware available in cache:");
            
            Console.WriteLine();

            // In Advanced/Debug mode, show all compatible versions as separate options
            if (OperatorSettings.IsAtLeast(OperatorMode.Advanced))
            {
                // Build a flat list of all device/bridge/firmware combinations
                var options = new List<(CANDevice device, byte bridgeId, FirmwareInfo firmware)>();
                
                foreach (var (device, bridgeId, versions) in devicesWithFirmware)
                {
                    int currentFirmware = device.BridgeFirmwareVersions.GetValueOrDefault(bridgeId, 0);
                    string currentVersion = currentFirmware > 0 ? CANDevice.ConvertFirmwareToString(currentFirmware) : "Not installed";
                    string bridgeName = GetBridgeName(bridgeId);
                    
                    // Show device header
                    Console.ForegroundColor = ConsoleColor.White;
                    Console.Write($"{device.PartNumber}");
                    Console.ForegroundColor = ConsoleColor.Gray;
                    Console.Write($" (Serial: {device.SerialString})");
                    Console.ForegroundColor = ConsoleColor.Yellow;
                    Console.WriteLine($" - Bridge {bridgeId} ({bridgeName})");
                    Console.ResetColor();
                    
                    // Show each firmware version as a separate option
                    foreach (var firmware in versions)
                    {
                        int optionNumber = options.Count + 1;
                        options.Add((device, bridgeId, firmware));
                        
                        int fwInt = CANDevice.ConvertFirmwareToInt(firmware.VersionString);
                        bool isNewer = currentFirmware > 0 && fwInt > currentFirmware;
                        bool isSame = currentFirmware > 0 && fwInt == currentFirmware;
                        bool isOlder = currentFirmware > 0 && fwInt < currentFirmware;
                        
                        Console.ForegroundColor = ConsoleColor.Cyan;
                        Console.Write($"     {optionNumber}. ");
                        Console.ResetColor();
                        Console.Write($"Current: ");
                        Console.ForegroundColor = currentFirmware > 0 ? ConsoleColor.Yellow : ConsoleColor.DarkGray;
                        Console.Write($"v{currentVersion}");
                        Console.ResetColor();
                        Console.Write(" \u2192 ");
                        
                        // Color code based on version comparison
                        if (isSame)
                            Console.ForegroundColor = ConsoleColor.Gray;
                        else if (isNewer)
                            Console.ForegroundColor = ConsoleColor.Green;
                        else if (isOlder)
                            Console.ForegroundColor = ConsoleColor.DarkYellow;
                        else
                            Console.ForegroundColor = ConsoleColor.White;
                        
                        Console.Write($"v{firmware.VersionString}");
                        
                        // Show prototype indicator in Advanced/Debug mode
                        if (firmware.IsPrototype)
                        {
                            Console.ForegroundColor = ConsoleColor.Magenta;
                            Console.Write(" [PROTOTYPE]");
                        }
                        
                        // Show indicator
                        if (isSame)
                        {
                            Console.ForegroundColor = ConsoleColor.DarkGray;
                            Console.Write(" (same)");
                        }
                        else if (isNewer)
                        {
                            Console.ForegroundColor = ConsoleColor.Green;
                            Console.Write(" (upgrade)");
                        }
                        else if (isOlder)
                        {
                            Console.ForegroundColor = ConsoleColor.DarkYellow;
                            Console.Write(" (downgrade)");
                        }
                        
                        Console.ResetColor();
                        Console.WriteLine();
                    }
                    
                    Console.WriteLine();
                }
                
                Console.Write($"  Select firmware (1-{options.Count}) or Q to cancel: ");
                
                int? selection = ReadNumericSelection(options.Count);
                if (!selection.HasValue)
                    return null;
                
                var selectedOption = options[selection.Value - 1];
                // Return as a list with single firmware version
                return (selectedOption.device, selectedOption.bridgeId, new List<FirmwareInfo> { selectedOption.firmware });
            }
            else
            {
                // Simple mode: Show only latest version per device/bridge
                for (int i = 0; i < devicesWithFirmware.Count; i++)
                {
                    var (device, bridgeId, versions) = devicesWithFirmware[i];
                    int currentFirmware = device.BridgeFirmwareVersions.GetValueOrDefault(bridgeId, 0);
                    string currentVersion = currentFirmware > 0 ? CANDevice.ConvertFirmwareToString(currentFirmware) : "Not installed";
                    string latestVersion = versions.First().VersionString;
                    string bridgeName = GetBridgeName(bridgeId);

                    Console.ForegroundColor = ConsoleColor.Cyan;
                    Console.Write($"  {i + 1}. ");
                    Console.ForegroundColor = ConsoleColor.White;
                    Console.Write($"CAN ID {device.CanId} - {device.PartNumber} - ");
                    Console.ForegroundColor = ConsoleColor.Yellow;
                    Console.Write($"Bridge {bridgeId} ({bridgeName})");
                    Console.ResetColor();
                    Console.WriteLine();

                    Console.ForegroundColor = ConsoleColor.Gray;
                    Console.WriteLine($"     Serial: {device.SerialString}");
                    Console.Write($"     Current: ");
                    Console.ForegroundColor = currentFirmware > 0 ? ConsoleColor.Red : ConsoleColor.DarkGray;
                    Console.Write($"v{currentVersion}");
                    Console.ResetColor();
                    
                    // Show available firmware with arrow
                    Console.Write(" \u2192 ");
                    Console.ForegroundColor = ConsoleColor.Green;
                    Console.Write($"v{latestVersion}");
                    Console.ResetColor();
                    
                    if (versions.Count > 1)
                    {
                        Console.ForegroundColor = ConsoleColor.DarkGray;
                        Console.Write($" ({versions.Count} versions)");
                        Console.ResetColor();
                    }
                    
                    Console.WriteLine();
                    Console.WriteLine();
                }

                Console.Write($"  Select device/bridge (1-{devicesWithFirmware.Count}) or Q to cancel: ");

                int? selection = ReadNumericSelection(devicesWithFirmware.Count);
                if (!selection.HasValue)
                    return null;

                return devicesWithFirmware[selection.Value - 1];
            }
        }

        private FirmwareInfo SelectFirmwareVersion(List<FirmwareInfo> availableVersions, CANDevice device, byte bridgeId)
        {
            if (availableVersions.Count == 1)
                return availableVersions[0];

            ConsoleHelper.WriteHeader("Select Firmware Version");
            
            int currentFirmware = device.BridgeFirmwareVersions.GetValueOrDefault(bridgeId, 0);
            string currentVersion = currentFirmware > 0 ? CANDevice.ConvertFirmwareToString(currentFirmware) : "Not installed";
            
            ConsoleHelper.WriteInfo($"Current firmware: v{currentVersion}");
            Console.WriteLine();
            ConsoleHelper.WriteInfo("Available versions:");
            Console.WriteLine();

            for (int i = 0; i < availableVersions.Count; i++)
            {
                var fw = availableVersions[i];
                int fwInt = CANDevice.ConvertFirmwareToInt(fw.VersionString);
                
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.Write($"  {i + 1}. ");
                Console.ForegroundColor = ConsoleColor.White;
                Console.Write($"v{fw.VersionString}");
                
                // Mark latest, current, or older versions
                if (i == 0) {
                  Console.ForegroundColor = ConsoleColor.Green;
                  Console.Write(" (Latest)");
                } else if (fwInt == currentFirmware) {
                  Console.ForegroundColor = ConsoleColor.Yellow;
                  Console.Write(" (Current)");
                } else if (fwInt < currentFirmware) {
                  Console.ForegroundColor = ConsoleColor.DarkYellow;
                  Console.Write(" (Downgrade)");
                }
                
                // Show prototype indicator
                if (fw.IsPrototype)
                {
                    Console.ForegroundColor = ConsoleColor.Magenta;
                    Console.Write(" [PROTOTYPE]");
                }
                
                Console.ResetColor();;
                Console.ForegroundColor = ConsoleColor.Gray;
                Console.WriteLine($" - {fw.LastUpdated:yyyy-MM-dd}");
                Console.ResetColor();
            }

            Console.WriteLine();
            Console.Write($"  Select version (1-{availableVersions.Count}) or Q to cancel: ");

            int? selection = ReadNumericSelection(availableVersions.Count);
            if (!selection.HasValue)
                return null;

            FirmwareInfo selectedFw = availableVersions[selection.Value - 1];

            // Warn if downgrading
            int selectedFwInt = CANDevice.ConvertFirmwareToInt(selectedFw.VersionString);

            if (currentFirmware > 0 && selectedFwInt < currentFirmware)
            {
                Console.WriteLine();
                ConsoleHelper.WriteWarning($"WARNING: You are about to DOWNGRADE from v{currentVersion} to v{selectedFw.VersionString}");
                Console.Write("  Are you sure? (Y/N): ");
                
                // Read single key for confirmation
                while (true)
                {
                    var confirmKey = Console.ReadKey(true);
            
                    if (confirmKey.Key == ConsoleKey.Y)
                    {
                        Console.WriteLine("Y");
                        break;
                    }
                    else if (confirmKey.Key == ConsoleKey.N || confirmKey.Key == ConsoleKey.Escape)
                    {
                        Console.WriteLine("N");
                        ConsoleHelper.WriteInfo("Version selection cancelled.");
                        return null;
                    }
                }
            }

            return selectedFw;
        }

        private static int GetFirmwareHardwareLine(string versionString)
        {
            return CANDevice.ConvertFirmwareToInt(versionString) / 100;
        }

        private static bool IsFirmwareCompatibleWithDevice(CANDevice device, byte bridgeId, FirmwareInfo firmware)
        {
            if (device == null || firmware == null)
                return false;

            if (!string.Equals(device.PartNumber, firmware.PartNumber, StringComparison.OrdinalIgnoreCase))
                return false;

            if (firmware.BridgeId != bridgeId)
                return false;

            if (!device.BridgeFirmwareVersions.TryGetValue(bridgeId, out int currentFirmware) || currentFirmware == 0)
                return false;

            return (currentFirmware / 100) == GetFirmwareHardwareLine(firmware.VersionString);
        }

        private string GetBridgeName(byte bridgeId)
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

        private async Task<bool> PerformSingleFirmwareUpdate(CANDevice device, byte bridgeId, FirmwareInfo firmware)
        {
            string tempHexFilePath = null;

            try
            {
                // Validate serial number
                if (string.IsNullOrEmpty(device.SerialString) || device.SerialNumber == 0)
                {
                    ConsoleHelper.WriteError("Device does not have a valid serial number. Skipping.");
                    return false;
                }

                tempHexFilePath = await CreateTempFirmwareFileAsync(firmware);
                return await RunFirmwareUploaderAsync(device, bridgeId, firmware, tempHexFilePath, showComPortHint: false);
            }
            catch (Exception ex)
            {
                ConsoleHelper.WriteError($"Update failed: {ex.Message}");
                return false;
            }
            finally
            {
                // Always clean up temporary file
                CleanupTempFile(tempHexFilePath);
            }
        }

        private async Task PerformFirmwareUpdate(CANDevice selectedDevice, byte bridgeId, FirmwareInfo firmware)
        {
            string tempHexFilePath = null;

            try
            {
                // Load and decrypt firmware
                ConsoleHelper.WriteInfo("Loading firmware from encrypted cache...");
                tempHexFilePath = await CreateTempFirmwareFileAsync(firmware);

                ConsoleHelper.WriteSuccess($"Firmware loaded: {firmware.FileName}");
                Console.WriteLine();

                // Start update immediately (user already confirmed version selection or downgrade)
                ConsoleHelper.WriteInfo("Starting firmware update...");
                await Task.Delay(500);

                Console.WriteLine();

                bool success = await RunFirmwareUploaderAsync(selectedDevice, bridgeId, firmware, tempHexFilePath, showComPortHint: true);

                if (!success)
                {
                    ConsoleHelper.WriteError("Firmware update failed.");
                }
            }
            catch (Exception ex)
            {
                ConsoleHelper.WriteError($"Failed to load firmware: {ex.Message}");
            }
            finally
            {
                // Always clean up temporary file
                CleanupTempFile(tempHexFilePath);
                PressAnyKey();
            }
        }

        private async Task<string> CreateTempFirmwareFileAsync(FirmwareInfo firmware)
        {
            byte[] decryptedData = cache.LoadFirmwareData(firmware);
            string tempHexFilePath = Path.Combine(Path.GetTempPath(), firmware.FileName);
            await File.WriteAllBytesAsync(tempHexFilePath, decryptedData);
            return tempHexFilePath;
        }

        private async Task<bool> RunFirmwareUploaderAsync(CANDevice device, byte bridgeId, FirmwareInfo firmware, string tempHexFilePath, bool showComPortHint)
        {
            if (!IsFirmwareCompatibleWithDevice(device, bridgeId, firmware))
            {
                ConsoleHelper.WriteError("Selected firmware is not compatible with this product. Part number and firmware family XX.XX must match.");
                return false;
            }

            string comPort = CANPort.FindPortByVidPid(
                "0483", "5740",  // CP - IOT Modem
                "04D8", "000A"   // CP - CAN Modem
            );

            if (string.IsNullOrEmpty(comPort))
            {
                ConsoleHelper.WriteError("No COM port found.");
                if (showComPortHint && OperatorSettings.IsAtLeast(OperatorMode.Advanced))
                {
                    ConsoleHelper.WriteInfo("Supported devices:");
                    ConsoleHelper.WriteInfo("  - CP IOT Modem (VID:0483 PID:5740)");
                    ConsoleHelper.WriteInfo("  - CP CAN Modem (VID:04D8 PID:000A)");
                }
                return false;
            }

            var uploader = new FirmwareUploader();
            bool success = await uploader.UploadFirmwareAsync(
                comPort,
                device.CanId,
                device.SerialString,
                bridgeId,
                tempHexFilePath,
                false,
                alreadyInBootloader: false,
                bootloaderLogger: logger,
                userIdentification: Environment.UserName,
                originalVersion: GetOriginalBridgeVersion(device, bridgeId),
                targetVersion: firmware?.VersionString ?? "Unknown",
                partNumber: firmware?.PartNumber
            );

            return success;
        }

        private static string GetOriginalBridgeVersion(CANDevice device, byte bridgeId)
        {
            if (device.BridgeFirmwareVersions.TryGetValue(bridgeId, out int versionInt))
            {
                return CANDevice.ConvertFirmwareToString(versionInt);
            }

            return "Unknown";
        }

        private void CleanupTempFile(string filePath)
        {
            if (!string.IsNullOrEmpty(filePath) && File.Exists(filePath))
            {
                try
                {
                    File.Delete(filePath);
                    if (OperatorSettings.IsAtLeast(OperatorMode.Advanced))
                        ConsoleHelper.WriteInfo("Temporary files cleaned up.");
                }
                catch
                {
                    // Ignore cleanup errors
                }
            }
        }

        private void ShowCachedFirmware()
        {
            var firmware = cache.GetAllFirmware();

            if (firmware.Count == 0)
            {
                ConsoleHelper.WriteWarning("No firmware in cache.");
                PressAnyKey();
                return;
            }

            ConsoleHelper.WriteHeader($"Cached Firmware ({firmware.Count} entries)");
            
            if (OperatorSettings.IsAtLeast(OperatorMode.Advanced))
            {
                Console.WriteLine($"  Cache Location: {cache.GetCacheDirectory()}");
                Console.WriteLine($"  Encryption: AES-256 (Enabled)");
            }
            
            Console.WriteLine();

            foreach (var fw in firmware.OrderBy(f => f.PartNumber).ThenBy(f => f.BridgeId).ThenByDescending(f => f.Version))
            {
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine($"  {fw.PartNumber} - Bridge {fw.BridgeId} - V{fw.VersionString}");
                
                if (OperatorSettings.IsAtLeast(OperatorMode.Advanced))
                {
                    Console.ForegroundColor = ConsoleColor.Gray;
                    Console.WriteLine($"    Last Updated: {fw.LastUpdated:yyyy-MM-dd HH:mm:ss}");
                    Console.WriteLine($"    File:         {Path.GetFileName(fw.HexFilePath)}");

                    // Show encrypted file size
                    if (File.Exists(fw.HexFilePath))
                    {
                        var fileInfo = new FileInfo(fw.HexFilePath);
                        Console.WriteLine($"    Size:         {fileInfo.Length / 1024}KB (encrypted)");
                    }
                }

                Console.ResetColor();
                Console.WriteLine();
            }

            PressAnyKey();
        }

        private void PressAnyKey()
        {
            Console.WriteLine();
            Console.ForegroundColor = ConsoleColor.DarkGray;
            Console.WriteLine("  Press any key to continue...");
            Console.ResetColor();
            Console.ReadKey(true);
        }
    }
}