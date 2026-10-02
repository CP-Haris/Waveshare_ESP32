// src/ble/Logger.ts
export class Logger {
    private static getTimestamp(): string {
        const now = new Date();
        return now.toISOString().split('T')[1].split('.')[0]; // HH:MM:SS format
    }

    private static formatMessage(level: string, ...args: any[]): any[] {
        const timestamp = this.getTimestamp();
        return [`[${timestamp}] [${level}]`, ...args];
    }

    static debug(...args: any[]): void {
        console.debug(...this.formatMessage("DEBUG", ...args));
    }

    static info(...args: any[]): void {
        console.info(...this.formatMessage("INFO", ...args));
    }

    static warn(...args: any[]): void {
        console.warn(...this.formatMessage("WARN", ...args));
    }

    static error(...args: any[]): void {
        console.error(...this.formatMessage("ERROR", ...args));
    }
}
