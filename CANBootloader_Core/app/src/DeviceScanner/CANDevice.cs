using System;
using System.Collections.Generic;

namespace CANBootloaderConsole.DeviceScanner
{
    public class CANDevice
    {
        public byte CanId { get; set; }
        public string PartNumber { get; set; }
        public ulong SerialNumber { get; set; }
        public string SerialString { get; set; }
        
        // Store firmware version for each bridge (1-4)
        public Dictionary<byte, int> BridgeFirmwareVersions { get; set; }
        
        public DateTime LastSeen { get; set; }

        public CANDevice(byte canId)
        {
            CanId = canId;
            PartNumber = string.Empty;
            SerialNumber = 0;
            SerialString = string.Empty;
            BridgeFirmwareVersions = new Dictionary<byte, int>();
            LastSeen = DateTime.Now;
        }

        /// <summary>
        /// Converts integer firmware version (e.g., 103037) to string format (e.g., "10.30.37")
        /// </summary>
        public static string ConvertFirmwareToString(int firmwareInt)
        {
            if (firmwareInt == 0)
                return "00.00.00";

            // Format: XXYYZZ where XX=major, YY=minor, ZZ=patch
            int major = firmwareInt / 10000;
            int minor = (firmwareInt / 100) % 100;
            int patch = firmwareInt % 100;

            return $"{major:D2}.{minor:D2}.{patch:D2}";
        }

        /// <summary>
        /// Converts string firmware version (e.g., "10.30.37") to integer format (e.g., 103037)
        /// </summary>
        public static int ConvertFirmwareToInt(string firmwareString)
        {
            if (string.IsNullOrEmpty(firmwareString))
                return 0;

            var parts = firmwareString.Split('.');
            if (parts.Length != 3)
                return 0;

            if (!int.TryParse(parts[0], out int major) ||
                !int.TryParse(parts[1], out int minor) ||
                !int.TryParse(parts[2], out int patch))
                return 0;

            return major * 10000 + minor * 100 + patch;
        }

        /// <summary>
        /// Checks if a firmware update is compatible (same major.minor, only patch version can differ)
        /// Hardware version is indicated by major.minor (first 4 digits), only patch (last 2 digits) can be updated
        /// </summary>
        /// <param name="currentVersion">Current firmware version as integer</param>
        /// <param name="availableVersion">Available firmware version as integer</param>
        /// <returns>True if update is compatible and newer</returns>
        public static bool IsCompatibleUpdate(int currentVersion, int availableVersion)
        {
            if (currentVersion == 0 || availableVersion == 0)
                return false;

            // Extract major.minor (hardware version) - first 4 digits
            int currentHardwareVersion = currentVersion / 100;  // Remove last 2 digits (patch)
            int availableHardwareVersion = availableVersion / 100;  // Remove last 2 digits (patch)

            // Hardware versions must match exactly
            if (currentHardwareVersion != availableHardwareVersion)
                return false;

            // Available version must be newer
            return availableVersion > currentVersion;
        }
    }
}