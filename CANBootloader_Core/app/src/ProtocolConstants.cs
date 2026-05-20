namespace CANBootloaderConsole
{
    public static class Frame
    {
        public const int SOF = 0;
        public const int CMD = 1;
        public const int DATA_LENGTH = 2;
        public const int STATUS = 4;
        public const int CRC_HIGH = 5;
        public const int DATA = 7;
    }

    public static class Data_Info
    {
        public const int VERSION = 7;
        public const int DEVICE_ID = 9;
        public const int ERASE_PAGE_SIZE = 11;
        public const int MINIMUM_WRITE_BLOCK_SIZE = 13;
        public const int PROGRAM_FLASH_START = 15;
        public const int PROGRAM_FLASH_END = 19;
    }

    public static class Command
    {
        public const int READ_VERSION = 0;
        public const int READ_FLASH = 1;
        public const int WRITE_FLASH = 2;
        public const int ERASE_FLASH = 3;
        public const int CALC_CHECKSUM = 8;
        public const int RESET_DEVICE = 9;
    }
}