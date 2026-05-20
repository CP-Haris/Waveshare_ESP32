using System;
using System.Collections.Generic;

namespace CANBootloaderConsole
{
    public class UnitFirmware
    {
        public enum Errorcode
        {
            WAITING_FOR_RESPONSE = 0x00,
            BOOTLOADER_CMD_RESPONSE_COMMAND_SUCCESS = 0x01,
            BOOTLOADER_CMD_RESPONSE_UNSUPPORTED_COMMAND = 0xFF,
            BOOTLOADER_CMD_RESPONSE_BAD_ADDRESS = 0xFE,
            BOOTLOADER_CMD_RESPONSE_BAD_LENGTH = 0xFD,
            BOOTLOADER_CMD_RESPONSE_VERIFY_FAIL = 0xFC,
            BOOTLOADER_CMD_RESPONSE_BAD_CRC = 0xFB,
            BOOTLOADER_CMD_RESPONSE_TIMEOUT = 0xFA
        }

        public int CANDataRXFrameCount;
        public List<byte> CANDataRXBuffer;

        public Errorcode status { get; set; }
        public ushort version { get; set; }
        public ushort device_id { get; set; }
        public ushort erase_page_size { get; set; }
        public ushort minimum_write_block_size { get; set; }
        public uint program_flash_start { get; set; }
        public uint program_flash_end { get; set; }

        public UnitFirmware()
        {
            CANDataRXBuffer = new List<byte>();
            CANDataRXFrameCount = -1;
        }

        public void Clear()
        {
            status = Errorcode.WAITING_FOR_RESPONSE;
            version = 0;
            device_id = 0;
            erase_page_size = 0;
            minimum_write_block_size = 0;
            program_flash_start = 0;
            program_flash_end = 0;
            CANDataRXFrameCount = -1;
            CANDataRXBuffer.Clear();
        }
    }
}