using System.Collections.Generic;
using System.Threading.Tasks;

namespace CANBootloaderConsole
{
    public interface IFirmwareSource
    {
        Task<List<FirmwareInfo>> GetAllFirmwareAsync(IEnumerable<FirmwareInfo> existingFirmware = null);
        Task<FirmwareInfo> GetFirmwareAsync(string partNumber, byte bridgeId);
        Task<bool> TestConnectionAsync();
    }
}