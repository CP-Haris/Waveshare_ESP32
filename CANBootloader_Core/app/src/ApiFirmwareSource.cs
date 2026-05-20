using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Net.Http;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;

namespace CANBootloaderConsole
{
    public class ApiFirmwareSource : IFirmwareSource
    {
        private const int MaxParallelDownloads = 4;
        private readonly HttpClient httpClient;
        private readonly string apiKey;
        private readonly JsonSerializerOptions jsonOptions = new JsonSerializerOptions
        {
            PropertyNameCaseInsensitive = true
        };

        private class FirmwareCatalogResponse
        {
            public List<FirmwareCatalogItem> Items { get; set; }
        }

        private class FirmwareCatalogItem
        {
            public string PartNumber { get; set; }
            public byte BridgeId { get; set; }
            public string VersionString { get; set; }
            public int VersionInt { get; set; }
            public DateTime PublishDateUtc { get; set; }
            public bool IsPrototype { get; set; }
        }

        public ApiFirmwareSource(string apiBaseUrl, string apiKeyValue, int timeoutSeconds = 10)
        {
            if (string.IsNullOrWhiteSpace(apiBaseUrl))
                throw new ArgumentException("API base URL cannot be empty", nameof(apiBaseUrl));

            if (string.IsNullOrWhiteSpace(apiKeyValue))
                throw new ArgumentException("API key cannot be empty", nameof(apiKeyValue));

            apiKey = apiKeyValue;
            var normalizedBase = apiBaseUrl.TrimEnd('/');
            httpClient = new HttpClient
            {
                BaseAddress = new Uri(normalizedBase + "/"),
                Timeout = TimeSpan.FromSeconds(Math.Max(3, timeoutSeconds))
            };
        }

        public async Task<bool> TestConnectionAsync()
        {
            try
            {
                var response = await httpClient.GetAsync("api/v1/health/ready");
                return response.IsSuccessStatusCode;
            }
            catch
            {
                return false;
            }
        }

        public async Task<List<FirmwareInfo>> GetAllFirmwareAsync(IEnumerable<FirmwareInfo> existingFirmware = null)
        {
            var firmwareList = new List<FirmwareInfo>();

            try
            {
                var request = new HttpRequestMessage(HttpMethod.Get, "api/v1/firmware/catalog?includePrototype=true");
                request.Headers.Add("X-Api-Key", apiKey);

                var response = await httpClient.SendAsync(request);
                if (!response.IsSuccessStatusCode)
                {
                    ConsoleHelper.WriteWarning($"API catalog request failed: {(int)response.StatusCode}");
                    return firmwareList;
                }

                var json = await response.Content.ReadAsStringAsync();
                var catalog = JsonSerializer.Deserialize<FirmwareCatalogResponse>(json, jsonOptions);
                if (catalog?.Items == null || catalog.Items.Count == 0)
                    return firmwareList;

                var existingMap = (existingFirmware ?? Enumerable.Empty<FirmwareInfo>())
                    .GroupBy(f => (f.PartNumber ?? string.Empty).ToUpperInvariant() + "|" + f.BridgeId + "|" + f.VersionString)
                    .ToDictionary(g => g.Key, g => g.OrderByDescending(f => f.LastUpdated).First());

                bool IsUpToDateInCache(FirmwareCatalogItem item)
                {
                    string key = (item.PartNumber ?? string.Empty).ToUpperInvariant() + "|" + item.BridgeId + "|" + item.VersionString;
                    if (!existingMap.TryGetValue(key, out var cached))
                        return false;

                    // Cache is up to date when we already have this exact version with at least the same publish timestamp.
                    return cached.LastUpdated >= item.PublishDateUtc;
                }

                var itemsToDownload = catalog.Items
                    .Where(item => !IsUpToDateInCache(item))
                    .ToList();

                var semaphore = new SemaphoreSlim(MaxParallelDownloads);
                var downloadTasks = itemsToDownload.Select(async item =>
                {
                    await semaphore.WaitAsync();
                    try
                    {
                        return await DownloadFirmwareAsync(item);
                    }
                    finally
                    {
                        semaphore.Release();
                    }
                });

                var downloadedFirmware = await Task.WhenAll(downloadTasks);
                firmwareList.AddRange(downloadedFirmware.Where(f => f != null));

                int skipped = catalog.Items.Count - itemsToDownload.Count;
                if (skipped > 0)
                {
                    ConsoleHelper.WriteInfo($"Catalog entries: {catalog.Items.Count}, skipped up-to-date: {skipped}, downloaded: {firmwareList.Count}");
                }
                else
                {
                    ConsoleHelper.WriteInfo($"Retrieved {firmwareList.Count} firmware file(s) from API catalog ({catalog.Items.Count} entries)");
                }
            }
            catch (Exception ex)
            {
                ConsoleHelper.WriteError($"Failed to fetch firmware from API: {ex.Message}");
            }

            return firmwareList;
        }

        public async Task<FirmwareInfo> GetFirmwareAsync(string partNumber, byte bridgeId)
        {
            try
            {
                string path = $"api/v1/firmware/catalog?partNumber={Uri.EscapeDataString(partNumber)}&includePrototype=true";
                var request = new HttpRequestMessage(HttpMethod.Get, path);
                request.Headers.Add("X-Api-Key", apiKey);

                var response = await httpClient.SendAsync(request);
                if (!response.IsSuccessStatusCode)
                    return null;

                var json = await response.Content.ReadAsStringAsync();
                var catalog = JsonSerializer.Deserialize<FirmwareCatalogResponse>(json, jsonOptions);
                if (catalog?.Items == null)
                    return null;

                var selected = catalog.Items
                    .Where(i => string.Equals(i.PartNumber, partNumber, StringComparison.OrdinalIgnoreCase) && i.BridgeId == bridgeId)
                    .OrderByDescending(i => i.PublishDateUtc)
                    .FirstOrDefault();

                if (selected == null)
                    return null;

                return await DownloadFirmwareAsync(selected);
            }
            catch (Exception ex)
            {
                ConsoleHelper.WriteError($"Failed to fetch firmware from API: {ex.Message}");
                return null;
            }
        }

        private async Task<FirmwareInfo> DownloadFirmwareAsync(FirmwareCatalogItem item)
        {
            string version = Uri.EscapeDataString(item.VersionString ?? string.Empty);
            string path = $"api/v1/firmware/download?partNumber={Uri.EscapeDataString(item.PartNumber)}&bridgeId={item.BridgeId.ToString(CultureInfo.InvariantCulture)}&version={version}";

            var request = new HttpRequestMessage(HttpMethod.Get, path);
            request.Headers.Add("X-Api-Key", apiKey);

            var response = await httpClient.SendAsync(request);
            if (!response.IsSuccessStatusCode)
                return null;

            var bytes = await response.Content.ReadAsByteArrayAsync();
            if (bytes == null || bytes.Length == 0)
                return null;

            return new FirmwareInfo
            {
                PartNumber = item.PartNumber,
                BridgeId = item.BridgeId,
                Version = item.VersionInt,
                VersionString = item.VersionString,
                HexFileData = bytes,
                LastUpdated = item.PublishDateUtc,
                IsPrototype = item.IsPrototype
            };
        }
    }
}