using System;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Threading.Tasks;

namespace CANBootloaderConsole
{
    static class Program
    {
        private const string DefaultApiBaseUrl = "http://49.12.206.181/firmware-api";
        private const string DefaultApiKey = "ff871ffebf04c37e60bafbc9dfcca0fdaec9d82b20d0febf351bf0819b457f10";
        private const int DefaultApiTimeoutSeconds = 10;

        // Windows API for disabling QuickEdit mode (prevents mouse clicks from freezing console)
        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr GetStdHandle(int nStdHandle);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool GetConsoleMode(IntPtr hConsoleHandle, out uint lpMode);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool SetConsoleMode(IntPtr hConsoleHandle, uint dwMode);

        private const int STD_INPUT_HANDLE = -10;
        private const uint ENABLE_QUICK_EDIT_MODE = 0x0040;
        private const uint ENABLE_EXTENDED_FLAGS = 0x0080;

        static async Task<int> Main(string[] args)
        {
            // Ensure Unicode symbols render correctly on all platforms (incl. Windows console)
            Console.OutputEncoding = System.Text.Encoding.UTF8;

            // Disable QuickEdit mode on Windows to prevent mouse clicks from freezing console
            DisableQuickEditMode();
            
            PrintBanner();

            try
            {
                // Initialize cache
                var cache = new FirmwareCache();

                IFirmwareSource repository = null;
                IOperationLogger logger = null;

                try
                {
                    repository = new ApiFirmwareSource(DefaultApiBaseUrl, DefaultApiKey, DefaultApiTimeoutSeconds);
                    logger = new ApiOperationLogger(DefaultApiBaseUrl, DefaultApiKey, DefaultApiTimeoutSeconds);
                    ConsoleHelper.WriteInfo("Initializing firmware service...");
                    await logger.UploadCachedLogsAsync();
                }
                catch (Exception ex)
                {
                    ConsoleHelper.WriteWarning($"Data source initialization failed: {ex.Message}");
                    ConsoleHelper.WriteInfo("Running in offline mode with cached firmware only.");
                }

                if (OperatorSettings.IsAtLeast(OperatorMode.Advanced))
                {
                    ConsoleHelper.WriteSuccess($"Cache initialized: {cache.GetCacheDirectory()}");
                    Console.WriteLine();
                    ConsoleHelper.WriteInfo("Starting interactive mode...");
                }

                // Check for debug/non-interactive mode
                if (args.Length > 0 && args[0] == "--debug-mode")
                {
                    ConsoleHelper.WriteInfo("Running in DEBUG MODE - Non-interactive");
                    ConsoleHelper.WriteInfo("Put your breakpoint here and step through the code!");
                    
                    // Add your debug code here - example:
                    // var menu = new InteractiveMenu(cache, repository);
                    // await menu.UpdateFirmwareAsync();
                    
                    Console.WriteLine("Debug mode complete - press any key to exit");
                    if (Console.IsInputRedirected == false)
                        Console.ReadKey();
                    return 0;
                }

                // Start interactive menu
                var menu = new InteractiveMenu(cache, repository, logger);
                await menu.RunAsync();

                return 0;
            }
            catch (Exception ex)
            {
                Console.WriteLine();
                ConsoleHelper.WriteError($"Fatal error: {ex.Message}");
                Console.WriteLine(ex.StackTrace);
                return 1;
            }
        }

        static void DisableQuickEditMode()
        {
            // Only needed on Windows
            if (!OperatingSystem.IsWindows())
                return;

            try
            {
                IntPtr consoleHandle = GetStdHandle(STD_INPUT_HANDLE);
                if (consoleHandle == IntPtr.Zero)
                    return;

                if (!GetConsoleMode(consoleHandle, out uint consoleMode))
                    return;

                // Disable QuickEdit and Extended Flags
                consoleMode &= ~ENABLE_QUICK_EDIT_MODE;
                consoleMode &= ~ENABLE_EXTENDED_FLAGS;

                SetConsoleMode(consoleHandle, consoleMode);
            }
            catch
            {
                // Silently ignore if we can't disable QuickEdit mode
            }
        }

        static void PrintBanner()
        {
            // Get version from assembly (InformationalVersion includes git commit hash via MinVer)
            string version = "Unknown";
            try
            {
                var infoVersion = Assembly.GetExecutingAssembly()
                    .GetCustomAttribute<AssemblyInformationalVersionAttribute>()
                    ?.InformationalVersion;
                
                if (!string.IsNullOrEmpty(infoVersion))
                {
                    // MinVer sets InformationalVersion to e.g. "2.3.0" or "2.3.1-alpha.0.1+a1b2c3d"
                    // Show clean version + short commit hash if present
                    var parts = infoVersion.Split('+');
                    version = parts[0]; // semantic version
                    if (parts.Length > 1)
                        version += $" ({parts[1][..Math.Min(7, parts[1].Length)]})";
                }
                else
                {
                    version = Assembly.GetExecutingAssembly().GetName().Version?.ToString() ?? "Unknown";
                }
            }
            catch { }

            string titleLine = $"CAN Bootloader Console Application v{version}";
            string subtitleLine = "Interactive Firmware Management Tool";

            // Keep banner stable across varying version lengths and console widths.
            int minInnerWidth = 59;
            int desiredInnerWidth = Math.Max(minInnerWidth, Math.Max(titleLine.Length, subtitleLine.Length) + 2);
            int maxInnerWidth = desiredInnerWidth;

            try
            {
                if (!Console.IsOutputRedirected && Console.WindowWidth > 6)
                {
                    maxInnerWidth = Math.Max(minInnerWidth, Console.WindowWidth - 2);
                }
            }
            catch
            {
                maxInnerWidth = desiredInnerWidth;
            }

            int innerWidth = Math.Min(desiredInnerWidth, maxInnerWidth);
            titleLine = TruncateForWidth(titleLine, innerWidth);
            subtitleLine = TruncateForWidth(subtitleLine, innerWidth);

            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.WriteLine();
            Console.WriteLine($"╔{new string('═', innerWidth)}╗");
            Console.WriteLine($"║{new string(' ', innerWidth)}║");
            Console.WriteLine($"║{CenterText(titleLine, innerWidth)}║");
            Console.WriteLine($"║{CenterText(subtitleLine, innerWidth)}║");
            Console.WriteLine($"║{new string(' ', innerWidth)}║");
            Console.WriteLine($"╚{new string('═', innerWidth)}╝");
            Console.ResetColor();
            Console.WriteLine();
        }

        private static string CenterText(string text, int width)
        {
            if (string.IsNullOrEmpty(text))
                return new string(' ', width);

            if (text.Length >= width)
                return text;

            int totalPadding = width - text.Length;
            int leftPadding = totalPadding / 2;
            int rightPadding = totalPadding - leftPadding;
            return new string(' ', leftPadding) + text + new string(' ', rightPadding);
        }

        private static string TruncateForWidth(string text, int width)
        {
            if (string.IsNullOrEmpty(text) || text.Length <= width)
                return text;

            if (width <= 3)
                return text.Substring(0, width);

            return text.Substring(0, width - 3) + "...";
        }
    }
}