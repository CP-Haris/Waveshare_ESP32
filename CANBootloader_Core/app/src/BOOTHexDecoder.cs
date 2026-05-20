using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using static System.Runtime.InteropServices.JavaScript.JSType;

namespace CANBootloaderConsole
{
    public static class BOOTHexDecoder
    {
        public static List<byte[]> MSG_DataToSend(byte[] DecodedDataArr, uint AppStartAddres, int packageSize)
        {
            List<byte[]> package = new List<byte[]>();

            for (int i = 0; i < DecodedDataArr.Length; i = i + packageSize)
            {
                int datasize = DecodedDataArr.Length - i;
                if (datasize > packageSize)
                    datasize = packageSize;

                int newpackagesize = RoundByMultiple(datasize, 8);
                byte[] datapack = new byte[newpackagesize + 16];
                MaskArray(ref datapack, 0xFF);
                Array.Copy(DecodedDataArr, i, datapack, 16, datasize);

                for (int a = 16; a < datapack.Length; a++)
                {
                    if (datapack[a] != 0xFF)
                    {
                        datapack[0] = 0xAF;
                        datapack[1] = 0x02;

                        byte[] Size = BitConverter.GetBytes(newpackagesize);
                        datapack[2] = Size[0];
                        datapack[3] = Size[1];

                        datapack[4] = 0x55;
                        datapack[5] = 0x00;
                        datapack[6] = 0xAA;
                        datapack[7] = 0x00;

                        byte[] Adress = BitConverter.GetBytes(AppStartAddres + i);
                        datapack[8] = Adress[0];
                        datapack[9] = Adress[1];
                        datapack[10] = Adress[2];
                        datapack[11] = Adress[3];

                        datapack[12] = 0;
                        datapack[13] = 0;
                        datapack[14] = 0;
                        datapack[15] = 0;

                        ushort CRC = Functions.crc16(datapack, 1, datapack.Length);
                        datapack[15] = (byte)(CRC >> 8);
                        datapack[14] = (byte)CRC;

                        package.Add(datapack);
                        break;
                    }
                }
            }

            return package;
        }

        public static byte[] MSG_Request()
        {
            byte[] msg = new byte[] { 0xAF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            ushort CRC = Functions.crc16(msg, 1, msg.Length);
            msg[msg.Length - 1] = (byte)(CRC >> 8);
            msg[msg.Length - 2] = (byte)CRC;
            return msg;
        }

        public static byte[] MSG_Reset()
        {
            byte[] msg = new byte[] { 0xAF, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            ushort CRC = Functions.crc16(msg, 1, msg.Length);
            msg[msg.Length - 1] = (byte)(CRC >> 8);
            msg[msg.Length - 2] = (byte)CRC;
            return msg;
        }

        public static byte[] MSG_EraseFlash(uint ByteAddress, uint length)
        {
            byte[] msg = new byte[] { 0xAF, 0x03, 0x00, 0x00, 0x55, 0x00, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

            byte[] Adress = BitConverter.GetBytes(ByteAddress);
            msg[8] = Adress[0];
            msg[9] = Adress[1];
            msg[10] = Adress[2];
            msg[11] = Adress[3];

            byte[] LengthArr = BitConverter.GetBytes(length);
            msg[12] = LengthArr[0];
            msg[13] = LengthArr[1];

            ushort CRC = Functions.crc16(msg, 1, msg.Length);
            msg[msg.Length - 1] = (byte)(CRC >> 8);
            msg[msg.Length - 2] = (byte)CRC;
            return msg;
        }

        public static int RoundByMultiple(int value, int multiple)
        {
            return (int)Math.Round((value / (double)multiple), MidpointRounding.AwayFromZero) * multiple;
        }

        public static void MaskArray(ref byte[] Array, byte Char)
        {
            for (int i = 0; i < Array.Length; i++)
            {
                Array[i] = Char;
            }
        }

        public static byte[] HexFileToData(string HexText, uint AppStartAddres, uint AppEndAddres)
        {
            const uint DATA = 0;
            const uint EOF = 1;
            const uint SAR = 2;
            const uint ELA = 4;

            uint HighesAddress = 0;
            byte[] Data = new byte[AppEndAddres - AppStartAddres + 1];

            for (int i = 0; i < Data.Length; i++)
            {
                Data[i] = 0xFF;
            }

            bool BAD_CRC = false;

            using (StringReader reader = new StringReader(HexText))
            {
                uint countLine = 0;
                byte[] ExtendedAddress = { 0, 0 };
                uint DataAddress = 0;
                uint DataLenght = 0;
                uint DataType = 0;

                string line = null;
                while (((line = reader.ReadLine()) != null) && !BAD_CRC)
                {
                    countLine++;
                    byte[] buf = HexStringToByteArray(line.Substring(1));

                    if (Checksum(buf))
                    {
                        DataLenght = buf[0];
                        DataAddress = (uint)((ExtendedAddress[0] << 24) | (ExtendedAddress[1] << 16) | (buf[1] << 8) | buf[2]);
                        DataType = buf[3];

                        switch (DataType)
                        {
                            case DATA:
                                if ((DataAddress >= AppStartAddres) && (DataAddress <= AppEndAddres))
                                {
                                    uint AdressToWrite = DataAddress - AppStartAddres;
                                    uint SpaceLeft = AppEndAddres - DataAddress;

                                    if (SpaceLeft <= DataLenght)
                                        DataLenght = SpaceLeft;

                                    Array.Copy(buf, 4, Data, AdressToWrite, DataLenght);

                                    if ((AdressToWrite + DataLenght) > HighesAddress)
                                        HighesAddress = AdressToWrite + DataLenght;
                                }
                                break;
                            case EOF:
                                break;
                            case SAR:
                                break;
                            case ELA:
                                ExtendedAddress[0] = buf[4];
                                ExtendedAddress[1] = buf[5];
                                break;
                        }
                    }
                    else
                    {
                        BAD_CRC = true;
                        Console.WriteLine("BAD CRC at line: " + countLine);
                        break;
                    }
                }
            }

            if (!BAD_CRC)
            {
                byte[] CutData = new byte[HighesAddress];
                Array.Copy(Data, CutData, HighesAddress);
                return CutData;
            }
            else
            {
                Console.WriteLine("Unable to create data array");
                return null;
            }
        }

        private static bool Checksum(byte[] HexDataString)
        {
            byte[] Data = new byte[HexDataString.Length - 1];
            Array.Copy(HexDataString, Data, HexDataString.Length - 1);

            int chkSum = Data.Aggregate(0, (s, b) => s += b) & 0xff;
            chkSum = (0x100 - chkSum) & 0xff;

            return (chkSum == HexDataString[HexDataString.Length - 1]);
        }

        private static byte[] HexStringToByteArray(string hex)
        {
            int length = hex.Length / 2;
            byte[] bytes = new byte[length];

            for (int i = 0; i < length; i++)
            {
                bytes[i] = Convert.ToByte(hex.Substring(i * 2, 2), 16);
            }

            return bytes;
        }
    }
}