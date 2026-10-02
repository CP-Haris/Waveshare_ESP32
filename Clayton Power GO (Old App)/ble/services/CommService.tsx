// CommService.ts

import { bleService } from '../BLEService';
import { CP_SERVICE, CP_CHARACTERISTIC } from '../BLEConstants';
import { Buffer } from 'buffer';
import { Characteristic, Subscription } from 'react-native-ble-plx';
import { BleDataHandler } from './BLEDataHandler';
import { getParamInfo } from './ParamDefinitions';
import { decodeValue } from './ParamDecoding';
import { Logger } from '../../components/Logger';

interface SendMessageOptions {
    block: number;
    id: number;
    rawValue: number;
    cmd?: number; // If needed, defaults to some command byte 
}

class CommService {
    private deviceId: string;
    private listenSubscription: Subscription | null = null;
    private bleDataHandler: BleDataHandler;
    private lastErrorBuffer: string = '';
    // Enhanced messaging controls
    private messageThrottle: number = 50; // Minimum ms between processing notifications
    private lastNotificationTime: number = 0;
    private notificationBuffer: Buffer[] = [];
    private processingTimer: NodeJS.Timeout | null = null;

    private onMessageCallback: (message: string) => void = () => { };

    constructor(deviceId: string) {
        this.deviceId = deviceId;
        this.bleDataHandler = new BleDataHandler((cmd, block, id, rawValue) => {
            // Special handling for error messages
            if (cmd === 0x41 && block === 0xFF && id === 0xFF && rawValue === 1) {
                const errorBuffer = this.bleDataHandler.getLastErrorBuffer();
                if (errorBuffer !== this.lastErrorBuffer) {
                    this.lastErrorBuffer = errorBuffer;
                }
                this.onMessageCallback("ERROR_BUFFER:" + errorBuffer);
                return;
            }

            const info = getParamInfo(block, id);
            let decodedStr: string;
            if (info) {
                decodedStr = decodeValue(info.prefix, rawValue, info.desc);
            } else {
                decodedStr = `Unknown param (Block=${block}, ID=${id}) Raw=0x${rawValue.toString(16)}`;
            }
            this.onMessageCallback(decodedStr);
        });
    }

    // Get CRC statistics for diagnostics
    getCrcStats() {
        return this.bleDataHandler.getCrcStats();
    }

    // Process buffered notifications to reduce rapid processing issues
    private processNotificationBuffer() {
        if (this.notificationBuffer.length === 0) {
            this.processingTimer = null;
            return;
        }

        const buffer = this.notificationBuffer.shift();
        if (buffer) {
            try {
                for (let i = 0; i < buffer.length; i++) {
                    this.bleDataHandler.processByte(buffer[i]);
                }
            } catch (error) {
                Logger.warn(`Error processing buffered notification: ${error}`);
            }
        }

        // Schedule next processing if there are more buffers
        if (this.notificationBuffer.length > 0) {
            this.processingTimer = setTimeout(() => {
                this.processNotificationBuffer();
            }, this.messageThrottle);
        } else {
            this.processingTimer = null;
        }
    }

    async listenForMessages(onMessageReceived: (message: string) => void, useHex: boolean): Promise<void> {
        if (this.listenSubscription) {
            console.warn('Already listening for messages');
            return;
        }

        this.onMessageCallback = onMessageReceived;

        const onNotificationReceived = (error: Error | null, characteristic: Characteristic | null) => {
            if (error?.message === 'Operation was cancelled') {
                return;
            }
            if (error) {
                console.error('Error receiving notification:', error);
                return;
            }
            if (characteristic?.value) {
                try {
                    const buffer = Buffer.from(characteristic.value, 'base64');
                    const now = Date.now();
                    
                    // Reduce throttling to improve responsiveness while preventing overload
                    if (now - this.lastNotificationTime < this.messageThrottle && this.notificationBuffer.length < 10) {
                        // Buffer the notification for later processing (with buffer limit)
                        this.notificationBuffer.push(buffer);
                        
                        // Start processing if not already running
                        if (!this.processingTimer) {
                            this.processingTimer = setTimeout(() => {
                                this.processNotificationBuffer();
                            }, this.messageThrottle);
                        }
                    } else {
                        // Process immediately or if buffer is getting too full
                        this.lastNotificationTime = now;
                        for (let i = 0; i < buffer.length; i++) {
                            this.bleDataHandler.processByte(buffer[i]);
                        }
                    }
                    
                    // Log CRC stats periodically (every 100 messages)
                    const stats = this.getCrcStats();
                    if (stats.totalMessages > 0 && stats.totalMessages % 100 === 0) {
                        Logger.debug(`Communication Stats: ${stats.errorCount} CRC errors, ${stats.shortMessages} short messages in ${stats.totalMessages} total (${stats.errorRate.toFixed(1)}% error rate)`);
                    }
                } catch (error) {
                    Logger.warn(`Error processing notification: ${error}`);
                }
            }
        };

        this.listenSubscription = await bleService.setupNotifications(
            this.deviceId,
            CP_SERVICE.CLAYTON_COMM,
            CP_CHARACTERISTIC.CLAYTON_LISTEN,
            onNotificationReceived
        );
    }

    async sendMessage(options: SendMessageOptions): Promise<void> {
        const { block, id, rawValue, cmd = 0x80 } = options;
        const data = new Uint8Array(7);
        data[0] = cmd & 0xff;
        data[1] = block & 0xff;
        data[2] = id & 0xff;
        data[3] = rawValue & 0xff;
        data[4] = (rawValue >> 8) & 0xff;
        data[5] = (rawValue >> 16) & 0xff;
        data[6] = (rawValue >> 24) & 0xff;

        const framedMessage = this.bleDataHandler.sendMessage(data);

        const base64Data = Buffer.from(framedMessage).toString('base64');

        await bleService.writeToDeviceWithoutResponse(
            this.deviceId,
            CP_SERVICE.CLAYTON_COMM,
            CP_CHARACTERISTIC.CLAYTON_SEND,
            base64Data
        );
    }

    stopListening(): void {
        if (this.listenSubscription) {
            this.listenSubscription.remove();
            this.listenSubscription = null;
        }
        
        // Clear any pending processing
        if (this.processingTimer) {
            clearTimeout(this.processingTimer);
            this.processingTimer = null;
        }
        this.notificationBuffer = [];
        
        // Log final CRC stats when stopping
        const stats = this.getCrcStats();
        if (stats.totalMessages > 0) {
            Logger.info(`Final Communication Stats: ${stats.errorCount} CRC errors, ${stats.shortMessages} short messages in ${stats.totalMessages} total (${stats.errorRate.toFixed(1)}% error rate)`);
        }
    }

    async clearErrors(): Promise<void> {
        const CMD = 0x60;  // Command byte
        const data = new Uint8Array([CMD, CMD]);
        
        const framedMessage = this.bleDataHandler.sendMessage(data);

        const base64Data = Buffer.from(framedMessage).toString('base64');

        await bleService.writeToDeviceWithoutResponse(
            this.deviceId,
            CP_SERVICE.CLAYTON_COMM,
            CP_CHARACTERISTIC.CLAYTON_SEND,
            base64Data
        );
    }
}

export default CommService;