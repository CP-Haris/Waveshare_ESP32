import { Logger } from "../../components/Logger";
import { OnParsedMessageCallback } from './types';
import { getActiveErrors, formatError, ErrorDefinition } from './ErrorDefinitions';

const SOH = 0x01;
const EOT = 0x04;
const DLE = 0x10;
const ERROR_CMD = 0x41;
const DEBOUNCE_TIME = 2000; // 2 seconds

export class BleDataHandler {
    private SOH_Detected = false;
    private DLE_Detected = false;
    private Msg_Buffer: number[] = [];
    private Msg_Counter = 0;
    private onMessage: OnParsedMessageCallback;
    private lastErrorLogTime: number = 0;
    private lastErrorBuffer: string = '';
    // Enhanced diagnostics for CRC issues
    private crcErrorCount = 0;
    private totalMessageCount = 0;
    private lastCrcErrorTime = 0;
    // Rate limiting for short message warnings
    private shortMessageCount = 0;
    private lastShortMessageLogTime = 0;
    private readonly SHORT_MESSAGE_LOG_INTERVAL = 5000; // Log every 5 seconds
    // Rate limiting for error buffer debug logging
    private lastErrorBufferDebugTime = 0;
    private readonly ERROR_BUFFER_DEBUG_INTERVAL = 10000; // Log every 10 seconds if no changes

    constructor(onMessage: OnParsedMessageCallback) {
        this.onMessage = onMessage;
    }

    getLastErrorBuffer(): string {
        return this.lastErrorBuffer;
    }

    // Enhanced CRC diagnostics
    getCrcStats(): { errorCount: number; totalMessages: number; errorRate: number; shortMessages: number } {
        return {
            errorCount: this.crcErrorCount,
            totalMessages: this.totalMessageCount,
            errorRate: this.totalMessageCount > 0 ? (this.crcErrorCount / this.totalMessageCount) * 100 : 0,
            shortMessages: this.shortMessageCount
        };
    }

    private calculateCrc(data: number[], length: number): number {
        let crc = 0;
        for (let i = 0; i < length; i++) {
            crc ^= (data[i] & 0xFF) << 8;
            for (let j = 0; j < 8; j++) {
                if ((crc & 0x8000) !== 0) {
                    crc = (crc << 1) ^ 0x1021;
                } else {
                    crc <<= 1;
                }
                crc &= 0xFFFF;
            }
        }
        return crc & 0xFFFF;
    }

    private processErrorBuffer(errorBuffer: Uint8Array) {
        const now = Date.now();
        if (now - this.lastErrorLogTime < DEBOUNCE_TIME) {
            return;
        }
        this.lastErrorLogTime = now;

        // Convert buffer to hex string and store it
        const currentErrorBuffer = Array.from(errorBuffer)
            .map(byte => byte.toString(16).padStart(2, '0'))
            .join(' ');
        
        // Only log debug info if buffer changed or every 10 seconds
        const bufferChanged = currentErrorBuffer !== this.lastErrorBuffer;
        const shouldLogPeriodically = now - this.lastErrorBufferDebugTime > this.ERROR_BUFFER_DEBUG_INTERVAL;
        
        if (bufferChanged || shouldLogPeriodically) {
            this.lastErrorBufferDebugTime = now;
            
            if (bufferChanged) {
                Logger.debug(`Error Buffer Changed: ${currentErrorBuffer}`);
            } else {
                Logger.debug(`Error Buffer Status: ${currentErrorBuffer}`);
            }
            
            // Debug: Show each non-zero byte as a direct error code
            errorBuffer.forEach((byte, byteIndex) => {
                if (byte !== 0) {
                    Logger.debug(`Slot ${byteIndex}: Error Code ${byte} (0x${byte.toString(16).padStart(2, '0')})`);
                }
            });
        }
        
        this.lastErrorBuffer = currentErrorBuffer;

        // Logger.error(`Error buffer: ${this.lastErrorBuffer}`);

        // // Get and log active errors
        // const activeErrors = getActiveErrors(errorBuffer);
        // if (activeErrors.length > 0) {
        //     Logger.error('Active Errors:');
        //     activeErrors.forEach(error => {
        //         Logger.error(formatError(error));
        //     });
        // }
    }

    processByte(byte: number) {
        if (!this.DLE_Detected && byte === DLE) {
            this.DLE_Detected = true;
        } else if (!this.DLE_Detected && byte === SOH) {
            // Start of message
            this.Msg_Counter = 1;
            this.SOH_Detected = true;
            this.Msg_Buffer = [];
        } else if (!this.DLE_Detected && byte === EOT) {
            // End of message
            this.SOH_Detected = false;
            this.Msg_Counter--;
            this.totalMessageCount++;

            // Handle short messages with rate-limited logging
            if (this.Msg_Counter < 2) {
                this.shortMessageCount++;
                const now = Date.now();
                
                // Rate limit short message warnings (log every 5 seconds)
                if (now - this.lastShortMessageLogTime > this.SHORT_MESSAGE_LOG_INTERVAL) {
                    this.lastShortMessageLogTime = now;
                    Logger.debug(`Short messages: ${this.shortMessageCount} received (may be valid status updates)`);
                    
                    // Show message content for debugging
                    if (this.Msg_Buffer.length > 0) {
                        const messageHex = this.Msg_Buffer.slice(0, Math.min(this.Msg_Counter + 1, this.Msg_Buffer.length))
                            .map(b => (isNaN(b) ? 0 : b).toString(16).padStart(2, '0'))
                            .join(' ');
                        Logger.debug(`Short message content: ${messageHex}`);
                    }
                }
                
                // Reset and return - but don't treat as error
                this.Msg_Buffer = [];
                this.Msg_Counter = 0;
                this.SOH_Detected = false;
                this.DLE_Detected = false;
                return;
            }

            const messageLength = this.Msg_Counter - 2;
            
            // Ensure messageLength is valid
            if (messageLength <= 0) {
                Logger.debug("Zero message length - likely keepalive or status message");
                // Reset and return
                this.Msg_Buffer = [];
                this.Msg_Counter = 0;
                this.SOH_Detected = false;
                this.DLE_Detected = false;
                return;
            }

            // Ensure we have enough data in buffer
            if (this.Msg_Buffer.length < this.Msg_Counter + 1) {
                Logger.warn(`Buffer too short: ${this.Msg_Buffer.length} < ${this.Msg_Counter + 1}`);
                // Reset and return
                this.Msg_Buffer = [];
                this.Msg_Counter = 0;
                this.SOH_Detected = false;
                this.DLE_Detected = false;
                return;
            }

            const CRC_Calc = this.calculateCrc(this.Msg_Buffer.slice(1, 1 + messageLength), messageLength);
            
            // Check if we have valid CRC bytes in the buffer
            const crcLowIndex = this.Msg_Counter - 1;
            const crcHighIndex = this.Msg_Counter;
            
            if (crcLowIndex < 0 || crcHighIndex >= this.Msg_Buffer.length) {
                Logger.warn(`Invalid CRC indices: low=${crcLowIndex}, high=${crcHighIndex}, bufferLength=${this.Msg_Buffer.length}`);
                // Reset and return
                this.Msg_Buffer = [];
                this.Msg_Counter = 0;
                this.SOH_Detected = false;
                this.DLE_Detected = false;
                return;
            }

            const CRC_Read = (this.Msg_Buffer[crcHighIndex] << 8) + (this.Msg_Buffer[crcLowIndex]);

            // Validate that CRC values are numbers
            if (isNaN(CRC_Calc) || isNaN(CRC_Read)) {
                this.crcErrorCount++;
                Logger.warn(`Invalid CRC values: Calc=${CRC_Calc}, Read=${CRC_Read}`);
                // Reset and return
                this.Msg_Buffer = [];
                this.Msg_Counter = 0;
                this.SOH_Detected = false;
                this.DLE_Detected = false;
                return;
            }

            if (CRC_Calc === CRC_Read) {
                const completeMessage = new Uint8Array(this.Msg_Buffer.slice(0, this.Msg_Counter + 1));

                if (completeMessage.length > 7) {
                    const cmd = completeMessage[1];
                    const block = completeMessage[2];
                    const id = completeMessage[3];
                    const rawValue = (completeMessage[4]) | (completeMessage[5] << 8) | (completeMessage[6] << 16) | (completeMessage[7] << 24);

                    // Handle error command (0x41)
                    if (cmd === ERROR_CMD) {
                        // The next 8 bytes after the command are the error buffer
                        const errorBuffer = completeMessage.slice(2, 10);
                        this.processErrorBuffer(errorBuffer);
                        
                        // Send error buffer to message handler using special block/id
                        this.onMessage(ERROR_CMD, 0xFF, 0xFF, 1); // rawValue 1 indicates error buffer is available
                    } else {
                        this.onMessage(cmd, block, id, rawValue);
                    }
                } else {
                    Logger.debug("Valid message but insufficient data for full parsing");
                }
            } else {
                this.crcErrorCount++;
                const now = Date.now();
                
                // Enhanced CRC error logging with rate limiting (max once per second)
                if (now - this.lastCrcErrorTime > 1000) {
                    this.lastCrcErrorTime = now;
                    
                    // Log detailed CRC error information with proper bounds checking
                    const messageDataLength = Math.min(this.Msg_Counter + 1, this.Msg_Buffer.length);
                    const messageHex = this.Msg_Buffer.slice(0, messageDataLength)
                        .map(b => {
                            // Ensure byte is a valid number
                            const byte = isNaN(b) ? 0 : b;
                            return byte.toString(16).padStart(2, '0');
                        })
                        .join(' ');
                    
                    Logger.warn(`CRC Error #${this.crcErrorCount}: Expected 0x${CRC_Calc.toString(16)}, got 0x${CRC_Read.toString(16)}`);
                    Logger.warn(`Message data: ${messageHex}`);
                    Logger.warn(`Buffer info: length=${this.Msg_Buffer.length}, counter=${this.Msg_Counter}, messageLength=${messageLength}`);
                    Logger.warn(`Error rate: ${this.getCrcStats().errorRate.toFixed(1)}% (${this.crcErrorCount}/${this.totalMessageCount})`);
                    
                    // If error rate is too high, suggest potential solutions
                    if (this.getCrcStats().errorRate > 10) {
                        Logger.warn('High CRC error rate detected. Possible causes:');
                        Logger.warn('- Bluetooth interference or weak signal');
                        Logger.warn('- Device sending data too rapidly');
                        Logger.warn('- Hardware communication issues');
                    }
                }
            }

            // Reset
            this.Msg_Buffer = [];
            this.Msg_Counter = 0;
            this.SOH_Detected = false;
            this.DLE_Detected = false;
        } else if (this.SOH_Detected) {
            this.DLE_Detected = false;
            this.Msg_Buffer[this.Msg_Counter] = byte;
            this.Msg_Counter++;
        } else {
            this.DLE_Detected = false;
        }
    }

    sendMessage(SRC: Uint8Array): Uint8Array {
        const DST: number[] = [];
        DST.push(0x01);

        for (let i = 0; i < SRC.length; i++) {
            const b = SRC[i];
            if (b === 0x04 || b === 0x01 || b === 0x10) {
                DST.push(0x10);
            }
            DST.push(b);
        }

        const CRC16 = this.calculateCrc(Array.from(SRC), SRC.length);
        const CRC16_Arr = [CRC16 & 0xFF, (CRC16 >> 8) & 0xFF];

        for (let i = 0; i < 2; i++) {
            if (CRC16_Arr[i] === 0x04 || CRC16_Arr[i] === 0x01 || CRC16_Arr[i] === 0x10) {
                DST.push(0x10);
            }
            DST.push(CRC16_Arr[i]);
        }

        DST.push(0x04); // EOT

        return new Uint8Array(DST);
    }
}
