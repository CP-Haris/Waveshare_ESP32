using System;

namespace CANBootloaderConsole
{
    public static class Functions
    {
        public static byte[] HexStrToByteArray(string hex)
        {
            if (hex.Length % 2 == 1)
                throw new ArgumentException("The binary key cannot have an odd number of digits", nameof(hex));

            return Convert.FromHexString(hex);
        }

        public static ushort crc16(byte[] data, int from, int len)
        {
            int i;
            byte ch;
            byte CRC_Low, CRC_High;
            ushort crc16 = 0x6363;

            for (i = from; i < len; i++)
            {
                ch = data[i];
                CRC_Low = (byte)crc16;
                CRC_High = (byte)(crc16 >> 8);
                ch = (byte)(ch ^ CRC_Low);
                ch = (byte)(ch ^ (ch << 4));
                crc16 = (ushort)(CRC_High ^ (ch << 8) ^ (ch << 3) ^ (ch >> 4));
            }

            return crc16;
        }
    }
}