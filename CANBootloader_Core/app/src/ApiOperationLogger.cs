using System;
using System.IO;
using System.Linq;
using System.Net.Http;
using System.Reflection;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;

namespace CANBootloaderConsole
{
    public class ApiOperationLogger : IOperationLogger
    {
        private readonly HttpClient httpClient;
        private readonly string apiKey;
        private static readonly string CacheDirectory = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
            "CANBootloader",
            "BootloaderLogs"
        );

        private class LogEntry
        {
            public string UserIdentification { get; set; }
            public string ApplicationSn { get; set; }
            public string BootloaderSn { get; set; }
            public string BridgeId { get; set; }
            public string OriginalVersion { get; set; }
            public string UpdatedToVersion { get; set; }
            public string Status { get; set; }
            public string DeviceBootloaderVersion { get; set; }
            public string AppBootloaderVersion { get; set; }
            public string ProductType { get; set; }
            public DateTime Timestamp { get; set; }
        }

        public ApiOperationLogger(string apiBaseUrl, string apiKeyValue, int timeoutSeconds = 10)
        {
            if (string.IsNullOrWhiteSpace(apiBaseUrl))
                throw new ArgumentException("API base URL cannot be empty", nameof(apiBaseUrl));

            if (string.IsNullOrWhiteSpace(apiKeyValue))
                throw new ArgumentException("API key cannot be empty", nameof(apiKeyValue));

            apiKey = apiKeyValue;
            httpClient = new HttpClient
            {
                BaseAddress = new Uri(apiBaseUrl.TrimEnd('/') + "/"),
                Timeout = TimeSpan.FromSeconds(Math.Max(3, timeoutSeconds))
            };
        }

        public async Task<bool> LogBootloadAsync(
            string userIdentification,
            string applicationSn,
            string bootloaderSn,
            string bridgeId,
            string originalVersion,
            string updatedToVersion,
            string status,
            string deviceBootloaderVersion = null,
            string appBootloaderVersion = null,
            string productType = null)
        {
            if (string.IsNullOrEmpty(appBootloaderVersion))
                appBootloaderVersion = GetApplicationVersion();

            var entry = new LogEntry
            {
                UserIdentification = userIdentification ?? "Unknown",
                ApplicationSn = applicationSn,
                BootloaderSn = bootloaderSn,
                BridgeId = bridgeId,
                OriginalVersion = originalVersion,
                UpdatedToVersion = updatedToVersion,
                Status = status,
                DeviceBootloaderVersion = deviceBootloaderVersion,
                AppBootloaderVersion = appBootloaderVersion,
                ProductType = productType,
                Timestamp = DateTime.UtcNow
            };

            try
            {
                bool sent = await SendLogEntryAsync(entry);
                if (sent)
                    return true;
            }
            catch
            {
                // Fall back to local cache when API is unavailable
            }

            try
            {
                SaveLocalLog(entry);
                return true;
            }
            catch
            {
                return false;
            }
        }

        public async Task<int> UploadCachedLogsAsync()
        {
            if (!Directory.Exists(CacheDirectory))
                return 0;

            var logFiles = Directory.GetFiles(CacheDirectory, "bootlog_*.json");
            if (logFiles.Length == 0)
                return 0;

            int successCount = 0;
            foreach (var logFile in logFiles)
            {
                try
                {
                    string json = File.ReadAllText(logFile);
                    var logEntry = JsonSerializer.Deserialize<LogEntry>(json);
                    if (logEntry == null)
                    {
                        File.Delete(logFile);
                        continue;
                    }

                    if (await SendLogEntryAsync(logEntry))
                    {
                        File.Delete(logFile);
                        successCount++;
                    }
                }
                catch
                {
                    // Keep file for next attempt
                }
            }

            return successCount;
        }

        public async Task<(string PartNumber, string BridgeId, string Version)?> GetLastKnownFirmwareAsync(string serialNumber)
        {
            if (string.IsNullOrEmpty(serialNumber) || serialNumber == "25757575755")
                return null;

            string formattedSerial = serialNumber.Replace("-", "").PadLeft(10, '0');

            // Try the API first (most authoritative — covers logs from all PCs)
            try
            {
                var request = new HttpRequestMessage(HttpMethod.Get, $"api/v1/bootloader/logs/last-known/{Uri.EscapeDataString(formattedSerial)}");
                request.Headers.Add("X-Api-Key", apiKey);
                var response = await httpClient.SendAsync(request);
                if (response.IsSuccessStatusCode)
                {
                    string json = await response.Content.ReadAsStringAsync();
                    using var doc = System.Text.Json.JsonDocument.Parse(json);
                    var root = doc.RootElement;
                    string partNumber = root.GetProperty("partNumber").GetString();
                    string bridgeId = root.GetProperty("bridgeId").GetString();
                    string version = root.GetProperty("version").GetString();
                    if (!string.IsNullOrEmpty(partNumber) && !string.IsNullOrEmpty(bridgeId))
                        return (partNumber, bridgeId, version);
                }
            }
            catch
            {
                // API unavailable — fall back to local cache
            }

            return GetLastKnownFirmwareFromCache(formattedSerial);
        }

        private async Task<bool> SendLogEntryAsync(LogEntry entry)
        {
            var payload = new
            {
                entry.UserIdentification,
                entry.ApplicationSn,
                entry.BootloaderSn,
                entry.BridgeId,
                entry.OriginalVersion,
                entry.UpdatedToVersion,
                entry.Status,
                entry.DeviceBootloaderVersion,
                entry.AppBootloaderVersion,
                entry.ProductType,
                TimestampUtc = entry.Timestamp
            };

            string json = JsonSerializer.Serialize(payload);
            var request = new HttpRequestMessage(HttpMethod.Post, "api/v1/bootloader/logs")
            {
                Content = new StringContent(json, Encoding.UTF8, "application/json")
            };

            request.Headers.Add("X-Api-Key", apiKey);

            var response = await httpClient.SendAsync(request);
            return response.IsSuccessStatusCode;
        }

        private void SaveLocalLog(LogEntry logEntry)
        {
            if (!Directory.Exists(CacheDirectory))
                Directory.CreateDirectory(CacheDirectory);

            string fileName = $"bootlog_{DateTime.UtcNow:yyyyMMdd_HHmmss_fff}.json";
            string filePath = Path.Combine(CacheDirectory, fileName);
            string json = JsonSerializer.Serialize(logEntry, new JsonSerializerOptions { WriteIndented = true });
            File.WriteAllText(filePath, json);
        }

        private (string PartNumber, string BridgeId, string Version)? GetLastKnownFirmwareFromCache(string serialNumber)
        {
            if (!Directory.Exists(CacheDirectory))
                return null;

            var logFiles = Directory.GetFiles(CacheDirectory, "bootlog_*.json");
            if (logFiles.Length == 0)
                return null;

            foreach (var logFile in logFiles.OrderByDescending(f => f))
            {
                try
                {
                    string json = File.ReadAllText(logFile);
                    var logEntry = JsonSerializer.Deserialize<LogEntry>(json);
                    if (logEntry == null)
                        continue;

                    if ((logEntry.ApplicationSn == serialNumber || logEntry.BootloaderSn == serialNumber) &&
                        !string.IsNullOrEmpty(logEntry.ProductType) &&
                        !string.IsNullOrEmpty(logEntry.BridgeId))
                    {
                        return (logEntry.ProductType, logEntry.BridgeId, logEntry.UpdatedToVersion);
                    }
                }
                catch
                {
                    // Skip malformed files
                }
            }

            return null;
        }

        private string GetApplicationVersion()
        {
            try
            {
                var version = Assembly.GetExecutingAssembly().GetName().Version;
                return version?.ToString() ?? "Unknown";
            }
            catch
            {
                return "Unknown";
            }
        }
    }
}