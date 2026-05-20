using System;
using System.IO;
using System.Security.Cryptography;
using System.Text;

namespace CANBootloaderConsole
{
    /// <summary>
    /// Provides AES-256 encryption/decryption for cached firmware files
    /// </summary>
    public static class CacheEncryption
    {
        private const int KeySize = 256;
        private const int IvSize = 16; // 128 bits
        private const int SaltSize = 32;
        private const int Iterations = 10000;

        /// <summary>
        /// Generates a consistent encryption key that works across different machines
        /// </summary>
        private static byte[] GetMachineKey()
        {
            // Use a fixed application-specific key that works across all platforms
            // This allows encrypted cache files to be shared between Windows and Mac
            string applicationKey = "CANBootloader_ClaytonPower_SecureCache_2024_CrossPlatform_Key";

            using (var sha256 = SHA256.Create())
            {
                return sha256.ComputeHash(Encoding.UTF8.GetBytes(applicationKey));
            }
        }

        /// <summary>
        /// Encrypts byte array data using AES-256
        /// </summary>
        /// <param name="plainData">Data to encrypt</param>
        /// <returns>Encrypted data with prepended salt and IV</returns>
        public static byte[] Encrypt(byte[] plainData)
        {
            if (plainData == null || plainData.Length == 0)
                throw new ArgumentException("Data to encrypt cannot be null or empty", nameof(plainData));

            try
            {
                // Generate random salt and IV
                byte[] salt = new byte[SaltSize];
                byte[] iv = new byte[IvSize];
                
                using (var rng = RandomNumberGenerator.Create())
                {
                    rng.GetBytes(salt);
                    rng.GetBytes(iv);
                }

                // Derive key from machine key + salt
                byte[] key = DeriveKey(GetMachineKey(), salt);

                using (var aes = Aes.Create())
                {
                    aes.KeySize = KeySize;
                    aes.Key = key;
                    aes.IV = iv;
                    aes.Mode = CipherMode.CBC;
                    aes.Padding = PaddingMode.PKCS7;

                    using (var encryptor = aes.CreateEncryptor())
                    using (var msEncrypt = new MemoryStream())
                    {
                        // Write salt and IV to the beginning of the stream
                        msEncrypt.Write(salt, 0, salt.Length);
                        msEncrypt.Write(iv, 0, iv.Length);

                        // Encrypt the data
                        using (var csEncrypt = new CryptoStream(msEncrypt, encryptor, CryptoStreamMode.Write))
                        {
                            csEncrypt.Write(plainData, 0, plainData.Length);
                            csEncrypt.FlushFinalBlock();
                        }

                        return msEncrypt.ToArray();
                    }
                }
            }
            catch (Exception ex)
            {
                throw new CryptographicException($"Encryption failed: {ex.Message}", ex);
            }
        }

        /// <summary>
        /// Decrypts byte array data using AES-256
        /// </summary>
        /// <param name="encryptedData">Encrypted data with prepended salt and IV</param>
        /// <returns>Decrypted plaintext data</returns>
        public static byte[] Decrypt(byte[] encryptedData)
        {
            if (encryptedData == null || encryptedData.Length < (SaltSize + IvSize))
                throw new ArgumentException("Encrypted data is invalid or corrupted", nameof(encryptedData));

            try
            {
                using (var msDecrypt = new MemoryStream(encryptedData))
                {
                    // Read salt and IV from the beginning of the stream
                    byte[] salt = new byte[SaltSize];
                    byte[] iv = new byte[IvSize];
                    
                    msDecrypt.Read(salt, 0, SaltSize);
                    msDecrypt.Read(iv, 0, IvSize);

                    // Derive key from machine key + salt
                    byte[] key = DeriveKey(GetMachineKey(), salt);

                    using (var aes = Aes.Create())
                    {
                        aes.KeySize = KeySize;
                        aes.Key = key;
                        aes.IV = iv;
                        aes.Mode = CipherMode.CBC;
                        aes.Padding = PaddingMode.PKCS7;

                        using (var decryptor = aes.CreateDecryptor())
                        using (var csDecrypt = new CryptoStream(msDecrypt, decryptor, CryptoStreamMode.Read))
                        using (var msPlain = new MemoryStream())
                        {
                            csDecrypt.CopyTo(msPlain);
                            return msPlain.ToArray();
                        }
                    }
                }
            }
            catch (CryptographicException ex)
            {
                throw new CryptographicException($"Decryption failed: {ex.Message}", ex);
            }
            catch (Exception ex)
            {
                throw new CryptographicException($"Decryption error: {ex.Message}", ex);
            }
        }

        /// <summary>
        /// Encrypts and saves data to a file
        /// </summary>
        public static void EncryptToFile(string filePath, byte[] plainData)
        {
            if (string.IsNullOrEmpty(filePath))
                throw new ArgumentException("File path cannot be null or empty", nameof(filePath));

            byte[] encryptedData = Encrypt(plainData);
            File.WriteAllBytes(filePath, encryptedData);
        }

        /// <summary>
        /// Loads and decrypts data from a file
        /// </summary>
        public static byte[] DecryptFromFile(string filePath)
        {
            if (string.IsNullOrEmpty(filePath))
                throw new ArgumentException("File path cannot be null or empty", nameof(filePath));

            if (!File.Exists(filePath))
                throw new FileNotFoundException($"Encrypted file not found: {filePath}");

            byte[] encryptedData = File.ReadAllBytes(filePath);
            return Decrypt(encryptedData);
        }

        /// <summary>
        /// Derives a cryptographic key using PBKDF2
        /// </summary>
        private static byte[] DeriveKey(byte[] password, byte[] salt)
        {
            using (var pbkdf2 = new Rfc2898DeriveBytes(password, salt, Iterations, HashAlgorithmName.SHA256))
            {
                return pbkdf2.GetBytes(KeySize / 8); // KeySize is in bits, GetBytes needs bytes
            }
        }

        /// <summary>
        /// Checks if a file appears to be encrypted (has valid salt+IV header)
        /// </summary>
        public static bool IsFileEncrypted(string filePath)
        {
            if (!File.Exists(filePath))
                return false;

            try
            {
                var fileInfo = new FileInfo(filePath);
                // Encrypted files must be at least salt + IV + some encrypted data
                return fileInfo.Length > (SaltSize + IvSize + 16);
            }
            catch
            {
                return false;
            }
        }
    }
}