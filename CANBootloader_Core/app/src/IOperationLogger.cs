using System.Threading.Tasks;

namespace CANBootloaderConsole
{
    public interface IOperationLogger
    {
        Task<bool> LogBootloadAsync(
            string userIdentification,
            string applicationSn,
            string bootloaderSn,
            string bridgeId,
            string originalVersion,
            string updatedToVersion,
            string status,
            string deviceBootloaderVersion = null,
            string appBootloaderVersion = null,
            string productType = null);

        Task<(string PartNumber, string BridgeId, string Version)?> GetLastKnownFirmwareAsync(string serialNumber);
        Task<int> UploadCachedLogsAsync();
    }
}