using System;

namespace CANBootloaderConsole
{
    public class FirmwareInfo
    {
        public string PartNumber { get; set; }
        public byte BridgeId { get; set; }
        public int Version { get; set; }
        public string VersionString { get; set; } // Store original version string (e.g., "11.32.73")
        public string HexFilePath { get; set; }
        public byte[] HexFileData { get; set; }
        public DateTime LastUpdated { get; set; }
        public bool IsPrototype { get; set; } // True if firmware is a prototype (not for production use)
        
        // Updated filename to use version string formatted as V##.##.##
        public string FileName => $"{PartNumber}_Bridge{BridgeId}_V{VersionString}.hex";
    }
}