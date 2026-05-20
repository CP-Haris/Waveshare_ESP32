using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Newtonsoft.Json;

namespace CANBootloaderConsole
{
    public class FirmwareCache
    {
        private readonly string cacheDirectory;
        private readonly string cacheIndexFile;
        private List<FirmwareInfo> cachedFirmware;
        private const string EncryptedFileExtension = ".enc"; // Encrypted files use .enc extension

        public FirmwareCache(string cacheDir = null)
        {
            cacheDirectory = cacheDir ?? Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
                "CANBootloader",
                "FirmwareCache"
            );

            cacheIndexFile = Path.Combine(cacheDirectory, "cache_index.json");
            
            // Ensure cache directory exists
            if (!Directory.Exists(cacheDirectory))
            {
                Directory.CreateDirectory(cacheDirectory);
            }

            LoadCacheIndex();
        }

        private void LoadCacheIndex()
        {
            cachedFirmware = new List<FirmwareInfo>();

            if (File.Exists(cacheIndexFile))
            {
                try
                {
                    string json = File.ReadAllText(cacheIndexFile);
                    var loadedFirmware = JsonConvert.DeserializeObject<List<FirmwareInfo>>(json) ?? new List<FirmwareInfo>();
                    
                    // Validate that hex files exist for loaded firmware
                    foreach (var fw in loadedFirmware)
                    {
                        if (!string.IsNullOrEmpty(fw.HexFilePath) && File.Exists(fw.HexFilePath))
                        {
                            cachedFirmware.Add(fw);
                        }
                        else
                        {
                            // Check if it might be an old unencrypted file that needs migration
                            ConsoleHelper.WriteWarning($"Cached file not found: {fw.HexFilePath}");
                        }
                    }
                }
                catch (Exception ex)
                {
                    ConsoleHelper.WriteWarning($"Failed to load cache index: {ex.Message}");
                }
            }
        }

        private void SaveCacheIndex()
        {
            try
            {
                // Don't save HexFileData to JSON (it's stored in separate encrypted files)
                var firmwareToSave = cachedFirmware.Select(f => new FirmwareInfo
                {
                    PartNumber = f.PartNumber,
                    BridgeId = f.BridgeId,
                    Version = f.Version,
                    VersionString = f.VersionString,
                    HexFilePath = f.HexFilePath,
                    HexFileData = null, // Don't serialize large binary data
                    LastUpdated = f.LastUpdated
                }).ToList();

                string json = JsonConvert.SerializeObject(firmwareToSave, Formatting.Indented);
                File.WriteAllText(cacheIndexFile, json);
            }
            catch (Exception ex)
            {
                ConsoleHelper.WriteError($"Failed to save cache index: {ex.Message}");
            }
        }

        public void AddOrUpdateFirmware(FirmwareInfo firmware)
        {
            try
            {
                // Check if this exact version already exists
                var existing = cachedFirmware.FirstOrDefault(f => 
                    f.PartNumber == firmware.PartNumber && 
                    f.BridgeId == firmware.BridgeId &&
                    f.Version == firmware.Version
                );

                // Save hex file to cache directory with .enc extension
                string hexFilePath = Path.Combine(cacheDirectory, firmware.FileName + EncryptedFileExtension);
                
                if (firmware.HexFileData != null && firmware.HexFileData.Length > 0)
                {
                    try
                    {
                        // Encrypt and save the firmware data
                        CacheEncryption.EncryptToFile(hexFilePath, firmware.HexFileData);
                        ConsoleHelper.WriteInfo($"Encrypted and cached: {firmware.FileName}");
                    }
                    catch (Exception ex)
                    {
                        ConsoleHelper.WriteError($"Failed to encrypt firmware file: {ex.Message}");
                        throw;
                    }
                }

                if (existing != null)
                {
                    // Update existing entry
                    existing.HexFilePath = hexFilePath;
                    existing.LastUpdated = firmware.LastUpdated != default ? firmware.LastUpdated : DateTime.UtcNow;
                    existing.VersionString = firmware.VersionString;
                    // Don't update HexFileData in memory to save RAM
                    existing.HexFileData = null;
                }
                else
                {
                    // Add new version
                    firmware.HexFilePath = hexFilePath;
                    firmware.LastUpdated = firmware.LastUpdated != default ? firmware.LastUpdated : DateTime.UtcNow;
                    // Don't keep HexFileData in memory to save RAM
                    firmware.HexFileData = null;
                    cachedFirmware.Add(firmware);
                }
                
                SaveCacheIndex();
            }
            catch (Exception ex)
            {
                ConsoleHelper.WriteError($"Failed to cache firmware {firmware.PartNumber} Bridge {firmware.BridgeId}: {ex.Message}");
            }
        }

        /// <summary>
        /// Loads and decrypts firmware data from cache when needed
        /// </summary>
        public byte[] LoadFirmwareData(FirmwareInfo firmware)
        {
            if (firmware == null)
                throw new ArgumentNullException(nameof(firmware));

            if (string.IsNullOrEmpty(firmware.HexFilePath))
                throw new InvalidOperationException("Firmware has no file path");

            if (!File.Exists(firmware.HexFilePath))
                throw new FileNotFoundException($"Firmware file not found: {firmware.HexFilePath}");

            try
            {
                // Decrypt and load the firmware data
                byte[] decryptedData = CacheEncryption.DecryptFromFile(firmware.HexFilePath);
                return decryptedData;
            }
            catch (Exception ex)
            {
                ConsoleHelper.WriteError($"Failed to decrypt firmware file: {ex.Message}");
                throw;
            }
        }

        public FirmwareInfo GetFirmware(string partNumber, byte bridgeId)
        {
            // Get the latest version for the specified part number and bridge
            return cachedFirmware
                .Where(f => f.PartNumber == partNumber && f.BridgeId == bridgeId)
                .OrderByDescending(f => f.Version)
                .FirstOrDefault();
        }

        public List<FirmwareInfo> GetAllFirmwareVersions(string partNumber, byte bridgeId)
        {
            // Get all versions for the specified part number and bridge, sorted by version
            return cachedFirmware
                .Where(f => f.PartNumber == partNumber && f.BridgeId == bridgeId)
                .OrderByDescending(f => f.Version)
                .ToList();
        }

        public List<FirmwareInfo> GetAllFirmware()
        {
            return cachedFirmware.ToList();
        }

        public string GetCacheDirectory()
        {
            return cacheDirectory;
        }

        public int GetCachedFirmwareCount()
        {
            return cachedFirmware.Count;
        }

        /// <summary>
        /// Migrates old unencrypted cache files to encrypted format
        /// </summary>
        public void MigrateUnencryptedFiles()
        {
            if (OperatorSettings.IsAtLeast(OperatorMode.Advanced))
            {
                ConsoleHelper.WriteInfo("Checking for unencrypted cache files...");
            }
            
            int migratedCount = 0;

            var hexFiles = Directory.GetFiles(cacheDirectory, "*.hex");
            
            foreach (var hexFile in hexFiles)
            {
                try
                {
                    string encryptedPath = hexFile + EncryptedFileExtension;
                    
                    // Skip if already encrypted
                    if (File.Exists(encryptedPath))
                        continue;

                    // Read unencrypted data
                    byte[] plainData = File.ReadAllBytes(hexFile);
                    
                    // Encrypt and save
                    CacheEncryption.EncryptToFile(encryptedPath, plainData);
                    
                    // Delete old unencrypted file
                    File.Delete(hexFile);
                    
                    migratedCount++;
                    ConsoleHelper.WriteInfo($"Migrated: {Path.GetFileName(hexFile)}");
                }
                catch (Exception ex)
                {
                    ConsoleHelper.WriteWarning($"Failed to migrate {Path.GetFileName(hexFile)}: {ex.Message}");
                }
            }

            if (migratedCount > 0)
            {
                ConsoleHelper.WriteSuccess($"Migrated {migratedCount} file(s) to encrypted format");
            }
            else
            {
                if (OperatorSettings.IsAtLeast(OperatorMode.Advanced))
                {
                    ConsoleHelper.WriteInfo("No unencrypted files found");
                }
            }
        }

        /// <summary>
        /// Clears all cached firmware files and index.
        /// </summary>
        public void ClearCache()
        {
            try
            {
                // Delete all encrypted files
                if (Directory.Exists(cacheDirectory))
                {
                    var files = Directory.GetFiles(cacheDirectory, "*" + EncryptedFileExtension);
                    foreach (var file in files)
                    {
                        try
                        {
                            File.Delete(file);
                        }
                        catch (Exception ex)
                        {
                            ConsoleHelper.WriteWarning($"Failed to delete {Path.GetFileName(file)}: {ex.Message}");
                        }
                    }

                    // Also delete any old unencrypted hex files
                    var hexFiles = Directory.GetFiles(cacheDirectory, "*.hex");
                    foreach (var file in hexFiles)
                    {
                        try
                        {
                            File.Delete(file);
                        }
                        catch (Exception ex)
                        {
                            ConsoleHelper.WriteWarning($"Failed to delete {Path.GetFileName(file)}: {ex.Message}");
                        }
                    }
                }

                // Delete cache index
                if (File.Exists(cacheIndexFile))
                {
                    File.Delete(cacheIndexFile);
                }

                // Clear in-memory list
                cachedFirmware.Clear();

                ConsoleHelper.WriteSuccess("Firmware cache cleared");
            }
            catch (Exception ex)
            {
                ConsoleHelper.WriteError($"Failed to clear cache: {ex.Message}");
            }
        }

        /// <summary>
        /// Gets the number of cached firmware entries.
        /// </summary>
        public int Count => cachedFirmware?.Count ?? 0;
    }
}