# CAN Bootloader Protocol - Detailed Technical Description

## Overview

The CAN Bootloader uses a robust state machine architecture to perform reliable firmware updates over a CAN bus network. The protocol implements a request-response pattern with automatic retry logic (up to 5 attempts) and CRC validation to ensure data integrity during the critical firmware update process.

---

## Communication Layer

### CAN Message Structure

All bootloader communication uses a proprietary protocol layered on top of J1939-style CAN messaging:

#### CAN Identifier (29-bit Extended Format)
```
Bootloader CAN ID: 0x19FF22
Message Format:    [0x19, 0xFF, 0x21, <Unit_ID>]
```

#### Message Fragmentation
Since CAN frames support a maximum of 8 bytes, larger bootloader messages are fragmented:
- **Byte 0**: Number of data bytes in this frame (1-7)
- **Bytes 1-7**: Payload data

Multiple CAN frames are reassembled on reception to form complete bootloader messages.

### Bootloader Message Frame Structure

Once reassembled, bootloader messages follow this format:

| Offset | Name | Size | Description |
|--------|------|------|-------------|
| 0 | SOF | 1 byte | Start of Frame (0xAF) |
| 1 | CMD | 1 byte | Command ID (0x00-0x09) |
| 2-3 | DATA_LENGTH | 2 bytes | Length of data payload (little-endian) |
| 4 | STATUS | 1 byte | Response status code |
| 5-6 | CRC | 2 bytes | CRC-16 checksum (little-endian) |
| 7+ | DATA | Variable | Command-specific data |

**Key Protocol Constants:**
```csharp
const BOOTLOADER_START_BYTE = 0xAF
const CAN_DATA_CHUNK_SIZE = 7  // Effective payload per CAN frame
```

### Command Codes

| Code | Name | Description |
|------|------|-------------|
| 0x00 | READ_VERSION | Request bootloader and device information |
| 0x01 | READ_FLASH | Read flash memory contents |
| 0x02 | WRITE_FLASH | Write data to flash memory |
| 0x03 | ERASE_FLASH | Erase flash memory pages |
| 0x08 | CALC_CHECKSUM | Calculate flash checksum |
| 0x09 | RESET_DEVICE | Reset device and jump to application |

### Status Codes

| Code | Name | Description |
|------|------|-------------|
| 0x00 | COMMAND_SUCCESS | Command executed successfully |
| 0x01 | UNSUPPORTED_COMMAND | Command not recognized |
| 0x02 | BAD_ADDRESS | Invalid memory address |
| 0x03 | BAD_LENGTH | Invalid data length |
| 0x04 | VERIFY_FAIL | Flash verification failed |
| 0x05 | BAD_CRC | CRC check failed |
| 0x06 | TIMEOUT | Bootloader timeout |

---

## State Machine Architecture

The bootloader operates as a deterministic state machine with 9 states. Each state performs a specific operation, validates the result, and transitions to the next state or handles errors appropriately.

```
┌─────────────────┐
│      Idle       │
└────────┬────────┘
         │ Start Upload
         ▼
┌─────────────────┐      ┌──────────────────┐
│  RequestInfo    │─────►│ GenerateHexData  │
└─────────────────┘      └────────┬─────────┘
         │                        │
         │ Error                  │
         ▼                        ▼
    ┌────────┐            ┌──────────────┐
    │Cleanup │◄───────────│  EraseFlash  │
    └────────┘            └──────┬───────┘
         ▲                       │
         │                       ▼
         │              ┌─────────────────┐
         │              │ HexDataToPack   │
         │              └────────┬────────┘
         │                       │
         │                       ▼
         │              ┌─────────────────┐
         │              │   WriteData     │◄─┐
         │              └────────┬────────┘  │
         │                       │           │
         │                       │  More     │
         │                       │  Packets  │
         │                       ▼           │
         │              ┌─────────────────┐  │
         │              │ WriteDataLast   │──┘
         │              └────────┬────────┘
         │                       │
         │                       ▼
         │              ┌─────────────────┐
         └──────────────│    GoToApp      │
                        └─────────────────┘
```

---

## State-by-State Protocol Details

### State 1: RequestInfo

**Purpose**: Query the bootloader for device configuration and capabilities.

**Protocol Message Sent:**
```
┌────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┐
│ AF │ 00 │ 00 │ 00 │ 00 │ 00 │CRC │CRC │ 00 │ 00 │ 00 │ 00 │ 00 │ 00 │ 00 │ 00 │
└────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┘
  SOF CMD  LENGTH  STAT     CRC         (Padding)
```

**Expected Response Structure:**
```
Data Offset  | Field                    | Size   | Description
-------------|--------------------------|--------|----------------------------------
7            | VERSION                  | 2 bytes| Bootloader version (e.g., 0x0100 = v1.0)
9            | DEVICE_ID                | 2 bytes| Device/processor identifier
11           | ERASE_PAGE_SIZE          | 2 bytes| Flash erase page size in bytes
13           | MINIMUM_WRITE_BLOCK_SIZE | 2 bytes| Minimum writable block size
15           | PROGRAM_FLASH_START      | 4 bytes| Application flash start address
19           | PROGRAM_FLASH_END        | 4 bytes| Application flash end address
```

**Code Implementation:**
```csharp
WriteData(CAN_ID, BOOTHexDecoder.MSG_Request());
```

**Next State:**
- **Success**: `GenerateHexData` - Bootloader info received, proceed to parse firmware
- **Failure**: `Cleanup` - Communication failed, abort operation

**Critical Validations:**
- Response must be exactly 16 bytes of data
- `ERASE_PAGE_SIZE` must not be zero
- `PROGRAM_FLASH_END` must be greater than `PROGRAM_FLASH_START`
- CRC must be valid

**What Happens:**
1. PC sends REQUEST_VERSION command (0x00)
2. Bootloader responds with device configuration
3. PC parses and validates memory layout and capabilities
4. Parameters are stored in `Bootloader` structure for use in subsequent states

---

### State 2: GenerateHexData

**Purpose**: Parse the Intel HEX firmware file and convert it to a raw binary format suitable for the target device's memory map.

**Protocol Activity**: None (local file processing only)

**Code Implementation:**
```csharp
string hexText = File.ReadAllText(HexFilePath);
hexData = BOOTHexDecoder.HexFileToData(hexText, 
                                       Bootloader.program_flash_start, 
                                       Bootloader.program_flash_end);
```

**Next State:**
- **Success**: `EraseFlash` - Data prepared, ready to erase target flash
- **Failure**: `Cleanup` - File read error or invalid HEX format

**Intel HEX Format Processing:**

The bootloader supports standard Intel HEX format with these record types:
- **00**: Data Record - Contains firmware bytes
- **01**: End of File - Marks end of HEX file
- **02**: Extended Segment Address - 20-bit addressing (rarely used)
- **04**: Extended Linear Address - 32-bit addressing (most common)

**Example HEX Record:**
```
:10010000214601360121470136007EFE09D2194097
│││││││││└───────────────────────┘└─┘
││││││││└─ Data bytes (16 bytes)   └─ Checksum
│││││││└─ Record Type (00 = Data)
││││││└─ Address High Byte (01)
│││││└─ Address Low Byte (00)
││││└─ Byte Count (10 = 16 bytes)
│││└─ Record Start (:)
```

**Processing Steps:**
1. Read HEX file as text
2. Parse each line, validating checksum
3. Build memory map from `program_flash_start` to `program_flash_end`
4. Unused areas filled with 0xFF (erased flash state)
5. Extended Linear Address records update the address high word
6. Data records populate the memory array
7. Trim array to highest written address

**Validation:**
- All checksums must be valid
- Data must fit within device flash boundaries
- Resulting binary array must not be empty

**Output:**
- `hexData[]`: Raw binary firmware data ready for flash writing

---

### State 3: EraseFlash

**Purpose**: Erase the application flash memory area to prepare for new firmware.

**Protocol Message Sent:**
```
┌────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┐
│ AF │ 03 │ 00 │ 00 │ 55 │ 00 │ AA │ 00 │A_L │A_H │A_E │A_T │L_L │L_H │CRC │CRC │
└────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┘
  SOF CMD  LENGTH   Magic       Magic    Start Address   Pages   CRC
```

**Field Breakdown:**
- **Byte 1**: Command 0x03 (ERASE_FLASH)
- **Bytes 4-7**: Magic unlock sequence (0x55, 0x00, 0xAA, 0x00) - prevents accidental erase
- **Bytes 8-11**: Start address (little-endian) - `program_flash_start`
- **Bytes 12-13**: Number of pages to erase (little-endian)

**Code Implementation:**
```csharp
uint pagesToErase = (uint)Math.Ceiling((double)hexData.Length / Bootloader.erase_page_size);
WriteData(CAN_ID, BOOTHexDecoder.MSG_EraseFlash(Bootloader.program_flash_start, pagesToErase));
```

**Calculation Example:**
```
Firmware Size:     32,768 bytes
Erase Page Size:    2,048 bytes
Pages to Erase:        16 pages
```

**Next State:**
- **Success**: `HexDataToPack` - Flash erased, ready to prepare write packets
- **Success (Erase-Only Mode)**: `Cleanup` - Erase complete, no write needed
- **Failure**: `Cleanup` - Erase failed, abort operation

**Timeout Handling:**
Flash erase can take significant time (1-10 seconds depending on device). The protocol implements:
- Extended timeout for erase operation
- Automatic retry up to 5 times on timeout
- Status polling for erase completion

**Bootloader Actions:**
1. Validates magic unlock sequence
2. Verifies address is within valid flash range
3. Performs page-by-page erase operation
4. Verifies erase by reading back 0xFF pattern
5. Returns SUCCESS or error code

---

### State 4: HexDataToPack

**Purpose**: Split the raw firmware binary into CAN-friendly packets optimized for efficient transmission.

**Protocol Activity**: None (local data preparation only)

**Packet Structure:**

Each firmware packet has a 16-byte header followed by up to 240 bytes of data:

```
┌────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬─────────┐
│ AF │ 02 │S_L │S_H │ 55 │ 00 │ AA │ 00 │A_L │A_H │A_E │A_T │00  │00  │CRC │CRC │ DATA... │
└────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴─────────┘
  SOF CMD  SIZE(LE)  Magic       Magic    Flash Address   Resv    CRC    Firmware Data
                                                                          (240 bytes max)
```

**Field Details:**
- **Byte 0**: Start marker (0xAF)
- **Byte 1**: Command (0x02 = WRITE_FLASH)
- **Bytes 2-3**: Data size (little-endian, rounded to 8-byte multiple)
- **Bytes 4-7**: Magic unlock (0x55, 0x00, 0xAA, 0x00)
- **Bytes 8-11**: Flash destination address (little-endian)
- **Bytes 12-13**: Reserved (0x00)
- **Bytes 14-15**: CRC-16 of entire packet (bytes 1 to end)
- **Bytes 16+**: Firmware data (0-240 bytes)

**Code Implementation:**
```csharp
packData = BOOTHexDecoder.MSG_DataToSend(hexData, 
                                         Bootloader.program_flash_start, 
                                         240);  // Max payload per packet
```

**Packet Generation Algorithm:**

```csharp
for (int i = 0; i < hexData.Length; i += 240) {
    // 1. Determine packet size (up to 240 bytes)
    int dataSize = Math.Min(240, hexData.Length - i);
    
    // 2. Round up to 8-byte multiple (device write requirement)
    int roundedSize = RoundToMultiple(dataSize, 8);
    
    // 3. Create packet: 16-byte header + rounded data
    byte[] packet = new byte[16 + roundedSize];
    
    // 4. Fill extra bytes with 0xFF (erased flash pattern)
    if (roundedSize > dataSize)
        FillBytes(packet, 16 + dataSize, 0xFF);
    
    // 5. Copy firmware data
    Array.Copy(hexData, i, packet, 16, dataSize);
    
    // 6. Only create packet if it contains non-0xFF data
    if (ContainsValidData(packet)) {
        // 7. Build header with address, size, magic, CRC
        packet[0] = 0xAF;
        packet[1] = 0x02; // WRITE_FLASH
        packet[2] = (byte)(roundedSize & 0xFF);
        packet[3] = (byte)(roundedSize >> 8);
        packet[4-7] = [0x55, 0x00, 0xAA, 0x00];  // Magic
        
        uint address = program_flash_start + i;
        packet[8-11] = BitConverter.GetBytes(address);
        
        // 8. Calculate and store CRC
        ushort crc = CRC16(packet, 1, packet.Length);
        packet[14] = (byte)(crc & 0xFF);
        packet[15] = (byte)(crc >> 8);
        
        packData.Add(packet);
    }
}
```

**Next State:**
- **Success**: `WriteData` - Packets ready, begin transmission
- **Failure**: `Cleanup` - Packet preparation failed

**Optimizations:**
- Packets containing only 0xFF are skipped (already erased state)
- Data is aligned to device write block boundaries
- Maximum packet size balances efficiency vs. CAN bandwidth

**Example:**
```
Firmware Size:     32,768 bytes
Packet Size:          240 bytes
Total Packets:        137 packets
Last Packet:           88 bytes (rounded to 96)
```

---

### State 5: WriteData

**Purpose**: Transmit firmware packets sequentially to the bootloader, writing data to flash memory.

**Protocol Message**: Each packet from `packData[]` is sent one at a time

**Code Implementation:**
```csharp
if (packageNumber < packData.Count) {
    WriteData(CAN_ID, packData[packageNumber]);
    packageNumber++;
}
```

**Transmission Details:**

Each packet (up to 256 bytes) is fragmented into multiple CAN frames:

**CAN Frame Format:**
```
┌────┬────┬────┬────┬────┬────┬────┬────┐
│Len │D_0 │D_1 │D_2 │D_3 │D_4 │D_5 │D_6 │
└────┴────┴────┴────┴────┴────┴────┴────┘
 Count  7 bytes of packet data
```

**Example: Sending a 256-byte packet**
```
Packet Size:    256 bytes (16-byte header + 240 data)
Frames Needed:   37 CAN frames
  - Frames 1-36:  7 bytes each (252 bytes total)
  - Frame 37:     4 bytes (total 256 bytes)

Frame 1:  [0x07, packet[0..6]]
Frame 2:  [0x07, packet[7..13]]
...
Frame 36: [0x07, packet[245..251]]
Frame 37: [0x04, packet[252..255], 0x00, 0x00, 0x00]
```

**Protocol Flow:**

```
PC                                          Bootloader
│                                                 │
│──── Packet #0 (37 CAN frames) ────────────────►│
│                                                 │ Write to flash
│                                                 │ Verify write
│◄─── SUCCESS (0x00) ────────────────────────────│
│                                                 │
│──── Packet #1 (37 CAN frames) ────────────────►│
│                                                 │ Write to flash
│                                                 │ Verify write
│◄─── SUCCESS (0x00) ────────────────────────────│
│                                                 │
│  ... continues for all packets ...             │
```

**Next State:**
- **Success (More Packets)**: `WriteData` - Continue sending next packet
- **Success (All Sent)**: `WriteDataLast` - Proceed to verification
- **Failure**: `WriteData` - Retry current packet (up to 5 times)
- **Max Retries**: `Cleanup` - Abort operation

**Retry Logic:**

On timeout or CRC error:
1. Stop timeout timer
2. Increment retry counter
3. If retries < 5:
   - Resend same packet
   - Restart timeout timer
4. If retries >= 5:
   - Abort with error

**Bootloader Actions per Packet:**
1. Receive and reassemble CAN frames
2. Validate CRC-16 checksum
3. Verify magic unlock sequence
4. Check address is within valid range
5. Write data to flash memory
6. Read back and verify written data
7. Send SUCCESS or error response

**Status Messages:**
- Simple Mode: Progress bar showing percentage
- Advanced Mode: "Uploading Firmware... [123/137 packets]"
- Debug Mode: Full CAN trace with addresses and checksums

---

### State 6: WriteDataLast

**Purpose**: Re-send the first packet to verify flash integrity and ensure bootloader can read the complete firmware.

**Protocol Message**: Resends `packData[0]` (the first packet)

**Code Implementation:**
```csharp
packageNumber = 0;
WriteData(CAN_ID, packData[0]);
```

**Why Resend the First Packet?**

This is a verification mechanism:
1. **Flash Integrity Check**: Ensures the beginning of firmware (often containing critical vectors and startup code) was written correctly
2. **Bootloader Validation**: Confirms bootloader can successfully read and verify the complete firmware
3. **CRC Consistency**: Validates that the flash memory is stable and not corrupted

**Next State:**
- **Success**: `GoToApp` - Verification passed, safe to reset
- **Failure**: `WriteDataLast` - Retry verification (up to 5 times)
- **Max Retries**: `Cleanup` - Flash verification failed

**Protocol Flow:**
```
PC                                          Bootloader
│                                                 │
│──── Packet #0 (again) ────────────────────────►│
│                                                 │ Read existing flash
│                                                 │ Compare with packet
│                                                 │ Verify CRC matches
│◄─── SUCCESS ───────────────────────────────────│
```

**Critical Importance:**

Many firmware corruption issues occur at:
- Flash write boundaries
- Power fluctuations during write
- Partial writes due to timing issues

Verifying the first packet (which contains interrupt vectors and boot code) ensures the device will successfully boot the new firmware.

---

### State 7: GoToApp

**Purpose**: Command the bootloader to reset the device and jump to the newly installed application firmware.

**Protocol Message Sent:**
```
┌────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┐
│ AF │ 09 │ 00 │ 00 │ 00 │ 00 │ 00 │ 00 │ 00 │ 00 │ 00 │ 00 │ 00 │ 00 │CRC │CRC │
└────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┘
  SOF CMD(0x09)  (All zeros)                                         CRC
```

**Code Implementation:**
```csharp
Reset();  // Calls WriteData(CAN_ID, BOOTHexDecoder.MSG_Reset())
```

**Next State:**
- **Success**: `Cleanup` - Device reset command acknowledged
- **Failure**: `GoToApp` - Retry reset command
- **Max Retries**: `Cleanup` - Proceed to cleanup anyway (device may have reset)

**Bootloader Actions:**
1. Receive RESET_DEVICE command
2. Perform final flash validation
3. Send SUCCESS response
4. Disable bootloader mode
5. Reset microcontroller
6. Jump to application vector table
7. Application firmware starts executing

**Device Reset Sequence:**

```
                   Bootloader                        Application
                       │                                  │
  ─────────────────────┼──────────────────────────────────┼─────
                       │                                  │
  Reset Command ──────►│                                  │
                       │                                  │
                       │ 1. Validate Flash               │
                       │ 2. Set Boot Flag                │
                       │ 3. Send SUCCESS                 │
                       │                                  │
  SUCCESS ◄────────────│                                  │
                       │                                  │
                       │ 4. Trigger MCU Reset            │
                       │                                  │
                  ┌────┴────┐                             │
                  │ RESET   │                             │
                  └────┬────┘                             │
                       │                                  │
                       │ 5. Read Boot Flag               │
                       │ 6. Jump to App                  │
                       │                                  │
                       └─────────────────────────────────►│
                                                          │
                                    7. Application Starts │
                                                          │
                                       ┌──────────────────┴─────┐
                                       │ Initialize Hardware    │
                                       │ Start Application      │
                                       │ Resume Normal CAN Comms│
                                       └────────────────────────┘
```

**Timeout Handling:**

The reset command may not receive a response if the device resets immediately. The protocol handles this gracefully:
- Timeout is expected and not considered an error
- After max retries or timeout, proceed to Cleanup
- Device will boot into application firmware

**Verification:**

After reset, the application firmware should:
1. Initialize normally
2. Resume CAN bus communication
3. Respond to standard CAN messages (not bootloader protocol)

The upload software can optionally verify the new firmware is running by:
- Querying device firmware version
- Checking device responds to application-level CAN messages

---

### State 8: Cleanup

**Purpose**: Finalize the firmware update process, release resources, and report final status.

**Protocol Activity**: None (local cleanup only)

**Code Implementation:**
```csharp
// Stop any active timers
StopCommTimeout();

// Determine final status
if (fromFail) {
    status = "Bootloading Failed";
    ConsoleHelper.WriteError($"Firmware upload failed at state: {lastSentState}");
} else {
    status = "Bootloading Completed";
    ConsoleHelper.WriteSuccess("Firmware update successful!");
}

// Reset flags and state
eraseOnly = false;
stopPressed = false;
totalPackages = 1;
retryCount = 0;
activeState = StateMachine.Idle;
```

**Next State:**
- **Always**: `Idle` - Return to ready state

**Cleanup Actions:**

1. **Timer Management**
   - Stop communication timeout timer
   - Clear any pending timeout events

2. **Status Reporting**
   - Determine success or failure
   - Report final status to user
   - Log error details if failed

3. **Resource Release**
   - Clear packet buffers
   - Reset packet counters
   - Free memory allocations

4. **State Reset**
   - Clear error flags (`fromFail`, `stopPressed`)
   - Reset retry counter
   - Clear cached firmware data
   - Return to `Idle` state

**Error Reporting:**

If `fromFail == true`, the cleanup reports:
- Which state failed
- Last command sent
- Error code received
- Retry count reached

**Success Reporting:**

If `fromFail == false`:
- Total packets sent
- Total bytes written
- Upload duration
- Device reset confirmed

**Mode-Specific Output:**

| Mode | Output |
|------|--------|
| Simple | "✓ Update Complete" or "✗ Update Failed" |
| Advanced | Detailed status with statistics |
| Debug | Full transaction log with timing |

---

## Error Handling and Recovery

### Timeout and Retry Mechanism

**Timeout Values:**
- Windows: 1000ms per packet
- macOS/Linux: 3000ms per packet (slower serial performance)

**Retry Logic:**
```
Attempt 1 ──► Timeout ──► Retry 1 ──► Timeout ──► Retry 2 ──► ... ──► Retry 5 ──► ABORT
   1s             1s         1s           1s          1s              1s
```

**Code Flow:**
```csharp
private void OnCommTimeoutElapsed(object sender, ElapsedEventArgs e)
{
    if (retryCount < MAX_RETRIES) {
        retryCount++;
        ConsoleHelper.WriteWarning($"Timeout - Retrying ({retryCount}/{MAX_RETRIES})...");
        WriteData(lastSentId, lastSentBuffer);  // Resend last message
        RestartCommTimeout();
    } else {
        ConsoleHelper.WriteError("Maximum retries reached. Aborting operation.");
        fromFail = true;
        ProcessState(StateMachine.Cleanup);
    }
}
```

### CRC Validation

All messages use CRC-16 for data integrity:

**CRC Calculation:**
```csharp
ushort CRC16(byte[] data, int startIndex, int length) {
    ushort crc = 0xFFFF;
    for (int i = startIndex; i < startIndex + length - 2; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if ((crc & 0x0001) != 0)
                crc = (ushort)((crc >> 1) ^ 0xA001);
            else
                crc = (ushort)(crc >> 1);
        }
    }
    return crc;
}
```

**CRC Failure Handling:**
- Bootloader validates CRC on every message
- Invalid CRC triggers `BAD_CRC` status response
- PC automatically retries on CRC error
- After 5 failures, operation aborts

### State Transition Error Handling

Each state defines success and failure paths:

```csharp
nextStatePass = StateMachine.WriteData;    // On success
nextStateFail = StateMachine.WriteData;    // On failure (retry)
```

**Example Failure Paths:**

| Current State | Failure | Next State |
|--------------|---------|------------|
| RequestInfo | Timeout/CRC | Cleanup (abort) |
| EraseFlash | Bad status | Cleanup (abort) |
| WriteData | Timeout/CRC | WriteData (retry) |
| WriteDataLast | Verify fail | WriteDataLast (retry) |

### Platform-Specific Considerations

**Windows:**
- Fast serial communication
- Standard 1s timeout
- QuickEdit mode disabled (prevents console freezing)

**macOS/Linux:**
- Slower serial driver
- Extended 3s timeout
- DTR/RTS disabled for stability
- Buffer flushing after port open

---

## Security Considerations

### Magic Unlock Sequences

Critical operations require magic values to prevent accidental execution:

**Erase Flash:**
```
Bytes 4-7: [0x55, 0x00, 0xAA, 0x00]
```

**Write Flash:**
```
Bytes 4-7: [0x55, 0x00, 0xAA, 0x00]
```

These sequences prevent:
- Accidental erase due to corrupted messages
- Random CAN bus noise triggering flash operations
- Malformed packets causing unintended writes

### Address Validation

The bootloader validates all memory addresses:
- Must be within `PROGRAM_FLASH_START` to `PROGRAM_FLASH_END`
- Protects bootloader code from being overwritten
- Prevents writes to reserved memory regions

### CRC Integrity

Every message includes CRC-16:
- Detects transmission errors
- Prevents corrupted firmware installation
- Validates message authenticity

---

## Performance Characteristics

### Typical Upload Times

| Firmware Size | Packets | CAN Frames | Time (115200 baud) |
|--------------|---------|------------|-------------------|
| 8 KB | 34 | ~1,250 | ~15 seconds |
| 32 KB | 137 | ~5,000 | ~45 seconds |
| 64 KB | 274 | ~10,000 | ~90 seconds |

**Calculation:**
```
CAN Frame Time = (Start + ID(4 bytes) + Data(8 bytes) + CRC + End) / BaudRate
               ≈ 15 bytes × 10 bits/byte / 115200 baud
               ≈ 1.3ms per frame

Total Time = (Frames × 1.3ms) + (Packets × TimeoutDelay) + EraseTime
```

### Bandwidth Utilization

```
Effective Data Rate = 240 bytes / (37 frames × 1.3ms) ≈ 5 KB/s
Protocol Overhead = Header(16 bytes) + CAN framing ≈ 7%
Flash Operations = Erase(~5s) + Write Verify(~20ms/packet)
```

---

## Debugging and Diagnostics

### Debug Mode Output

Enable Debug mode to see full protocol trace:

```
[TX] CAN ID: 19-FF-21-05
     Frame 1: [07, AF, 00, 00, 00, 00, 00, 3C]
     Frame 2: [07, B2, 00, 00, 00, 00, 00, 00]
     
[RX] CAN ID: 19-FF-22-05
     Frame 1: [07, AF, 00, 10, 00, 00, 3C, B2]
     Frame 2: [07, 00, 01, 00, 08, 00, 04, 00]
     
Bootloader Info Received:
  Version: 1.0
  Device ID: 0x0008
  Erase Page: 2048 bytes
  Flash: 0x00002000 - 0x0001FFFF
```

### Common Error Scenarios

| Error | Cause | Solution |
|-------|-------|----------|
| "Maximum retries reached" | CAN bus disconnected | Check physical connection |
| "Invalid erase page size" | Bootloader info corrupted | Reset device, retry |
| "Bad CRC detected" | Electrical noise on bus | Improve grounding, add filtering |
| "Verification failed" | Flash write error | Check power stability |
| "Access denied to port" | Port in use | Close other applications |

---

## Conclusion

The CAN Bootloader Protocol provides a robust, reliable firmware update mechanism over CAN bus networks. Key strengths include:

✓ **Reliability**: Automatic retry with CRC validation  
✓ **Safety**: Magic unlock sequences prevent accidental erase  
✓ **Efficiency**: Optimized packet sizes balance speed vs. overhead  
✓ **Debugging**: Comprehensive logging at multiple verbosity levels  
✓ **Cross-Platform**: Works on Windows, macOS, and Linux  
✓ **Fault Tolerance**: Graceful error handling and recovery  

The state machine architecture ensures deterministic behavior, making the protocol suitable for production firmware deployment in automotive, industrial, and embedded systems.
