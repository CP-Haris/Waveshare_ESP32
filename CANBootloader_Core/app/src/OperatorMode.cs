namespace CANBootloaderConsole
{
    /// <summary>
    /// Defines the operator mode levels for the application
    /// </summary>
    public enum OperatorMode
    {
        /// <summary>
        /// Simple mode - Minimal output, basic operations only
        /// </summary>
        Simple = 1,

        /// <summary>
        /// Advanced mode - Standard output with detailed information
        /// </summary>
        Advanced = 2
    }

    /// <summary>
    /// Global settings for operator mode
    /// </summary>
    public static class OperatorSettings
    {
        /// <summary>
        /// Current operator mode (default: Simple)
        /// </summary>
        public static OperatorMode CurrentMode { get; set; } = OperatorMode.Simple;

        /// <summary>
        /// Check if current mode is at least the specified level
        /// </summary>
        public static bool IsAtLeast(OperatorMode mode)
        {
            return CurrentMode >= mode;
        }

        /// <summary>
        /// Get display name for current mode
        /// </summary>
        public static string GetModeName()
        {
            return CurrentMode switch
            {
                OperatorMode.Simple => "Simple",
                OperatorMode.Advanced => "Advanced",
                _ => "Unknown"
            };
        }
    }
}