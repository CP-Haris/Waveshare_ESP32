# Firmware Update Bootloader

Dette dokument beskriver hvordan firmware update-flowet i ClaytonPowerApp virker. Fokus er den mobile implementation i `src/services/firmwareUpdateService.js`, som opdaterer LPS/BMS-moduler via BLE til ESP32-gateway og videre ud på CAN.

## Kort overblik

Firmware update i appen består af fem lag:

1. Firmware Update-skærmen finder target og viser update-planen.
2. `firmwareUpdateService` styrer hele bootloader-flowet og låser BLE-kommandokanalen mens update kører.
3. BLE-gatewayen på ESP32 modtager kommandoer fra appen.
4. ESP32 sender og modtager CAN frames i passthrough-mode.
5. Target device kører CAN bootloaderen og skriver firmware til flash.

Den vigtige praktiske pointe er, at appen ikke skriver direkte til target. Den sender BLE-kommandoer til ESP32-gatewayen, som oversætter dem til CAN frames.

## Passthrough vs. CAN ID

Passthrough er en BLE-gateway mode i ESP32'en: når den er slået til, kan appen sende og modtage de samme CAN frames som en CAN-adapter på bussen. Passthrough siger altså noget om transportvejen mellem app og ESP32, ikke om selve CAN ID'et.

Clayton Power CAN-protokollen er J1939-baseret og bruger 29-bit CAN IDs. Prefix som `0x18...` og `0x19...` er en del af J1939/CAN-adresseringen med prioritet, data page, PDU/PGN, destination og source address. De betyder ikke i sig selv, om data er rå, gateway-parset eller passthrough.

I firmware update-flowet kører CAN-trafikken som passthrough, efter appen har sendt BLE-kommandoen `SET_CAN_PASSTHROUGH`. Derefter sendes både discovery, application commands og bootloader packets som almindelige CAN frames via `SEND_CAN_FRAME` eller `SEND_CAN_FRAMES`.

## Moduler og bridge IDs

Bootloaderen bruger stadig bridge IDs internt. UI viser dog modulnavne ud fra partnumber-prefix:

| Partnumber | Bridge ID | Modulnavn |
| --- | ---: | --- |
| CLxxxx | 1 | Control |
| CLxxxx | 2 | Power |
| CLxxxx | 3 | Display |
| CLxxxx | 4 | DC/DC |
| CBxxxx | 1 | Control |

Hvis partnumber-prefix ikke kendes, falder UI tilbage til `Bridge X`. Dette er kun en visningsregel; update-flowet sender stadig det numeriske `bridgeId` til bootloaderen.

## BLE-gateway protokol

Appen bruger disse BLE UUID'er:

| Element | UUID |
| --- | --- |
| Service | `00001000-0000-1000-8000-00805f9b34fb` |
| TX notify | `00001001-0000-1000-8000-00805f9b34fb` |
| RX write | `00001002-0000-1000-8000-00805f9b34fb` |

De vigtigste BLE-kommandoer i bootloader-flowet er:

| Kommando | Værdi | Brug |
| --- | ---: | --- |
| `SET_CAN_PASSTHROUGH` | `0x18` | Slår CAN passthrough forwarding til/fra på ESP32-gatewayen. |
| `SEND_CAN_FRAME` | `0x19` | Sender en enkelt CAN frame. |
| `SEND_CAN_FRAMES` | `0x1A` | Sender flere CAN frames i en BLE write for højere uploadhastighed. |

CAN frames kommer tilbage som notify-message `CAN_FRAME = 0x08`.

`SEND_CAN_FRAMES` bruges til store `WRITE_FLASH`-pakker. Appen batcher op til 16 CAN frames per BLE write. Hvis batching giver timeout under write, slår appen batching fra og fortsætter med enkeltframes som fallback.

## CAN IDs

Clayton Power CAN-protokollen bruger 29-bit extended identifiers efter J1939-principper. I specifikationerne ses blandt andet broadcast/PDU2-adresser som `0x18FFiixx` og `0x19FFiixx`, hvor `ii` er message ID og `xx` er source/unit address. Peer-to-peer/PDU1-adresser bruger destination og source i de sidste bytes, for eksempel `0x18EFxxyy` i CAN-specifikationen.

Bootloaderen bruger samme 29-bit CAN ID-format. I implementationen behandles bootloaderens `0x19FF20`, `0x19FF21` og `0x19FF22` som top-24-bit base IDs, og helperen `canIdWithUnit(top24, unit)` sætter unit/source address i low byte.

| Formål | Top 24 bit | Fuld CAN ID |
| --- | ---: | --- |
| Bootloader status/control | `0x19FF20` | `(0x19FF20 << 8) | unitId` via helper `canIdWithUnit` |
| Host til bootloader data | `0x19FF21` | `(0x19FF21 << 8) | unitId` |
| Bootloader til host data | `0x19FF22` | `(0x19FF22 << 8) | unitId` |
| Application command | `0x19EFxx` | `0x19EF<targetCanId><hostId>` |
| Info broadcast | `0x18EAFFFE` | Bruges til target discovery |

Host/app bruger `PC_UNIT_ID = 0xFE` ved application-level kommandoer.

## Application-level boot og reset

For at få target fra normal applikation til bootloader sender appen en application command på CAN:

```text
CAN ID: 0x19EF<targetCanId>FE
Data:   [0x00, 0x01, 0, 0, 0, 0, 0, 0]
```

`0x01` betyder enter boot mode.

Derefter sendes reset-kommandoen:

```text
CAN ID: 0x19EF<targetCanId>FE
Data:   [0x00, 0x06, 0, 0, 0, 0, 0, 0]
```

`0x06` betyder reboot. Appen sender begge kommandoer i små bursts for at gøre overgangen robust.

Den samme `0x06` reset bruges også efter display-update, hvis bootloader-reset returnerer explicit `0xFA`. Det får display-applikationen til at starte uden at brugeren skal power-cycle enheden.

## Bootloader status/control frame

Status/control kører på `0x19FF20<unitId>`. Payload er altid 8 bytes:

| Byte | Indhold |
| ---: | --- |
| 0-4 | Serial bytes |
| 5 | Product ID |
| 6 | Bridge ID |
| 7 | Action/status |

Vigtige action/status værdier:

| Værdi | Navn | Brug |
| ---: | --- | --- |
| `199` | `BOOT_READY` | Bootloader heartbeat fra target. |
| `99` | `BOOT_ACK` | Appens ACK tilbage til target. |
| `0x01` | Serial/init action | Bruges når bootloader melder NO_SERIAL og appen skal kopiere serial. |
| `0x02` | Open bridge | Appen åbner ønsket modul/bridge før flash. |
| `0x04` | Exit/close bootloader bridge | Appen lukker bridge efter update. |

Mens update kører, ACK'er appen heartbeat frames. Det holder bootloaderen aktiv og matcher C#-programmets adfærd. Under selve `WRITE_FLASH`-payload transmission suspenderes heartbeat ACK kort, så CAN sendekøen ikke blandes med dataframes.

## Bootloader datapakker

Større bootloader beskeder pakkes i bootloader-payloads og fragmenteres over CAN.

Hver CAN data frame til bootloaderen ser sådan ud:

```text
Byte 0: Antal payload bytes i denne frame (1-7)
Byte 1-7: Op til 7 bytes bootloader payload
```

Bootloader payload starter altid med:

| Offset | Felt | Beskrivelse |
| ---: | --- | --- |
| 0 | `0xAF` | Start byte |
| 1 | Command | Bootloader command |
| 2-3 | Data length | Little endian |
| 4-13 | Command-specific header/data |
| 14-15 | CRC16 | Little endian |
| 16+ | Data | Kun for write packets |

CRC beregnes med seed `0x6363` fra offset 1. Før CRC beregnes sættes bytes 14 og 15 til `0x00`.

## Bootloader commands

| Command | Værdi | Brug |
| --- | ---: | --- |
| `READ_VERSION` | `0x00` | Henter bootloader-info, flash start/slut, erase page size. |
| `WRITE_FLASH` | `0x02` | Skriver en firmwareblok til flash. |
| `ERASE_FLASH` | `0x03` | Sletter applikationsflash før upload. |
| `RESET_DEVICE` | `0x09` | Resetter target efter upload. |

Responsstatus i appen:

| Status | Værdi | Betydning |
| --- | ---: | --- |
| `OK` | `0x01` | Kommando accepteret. |
| `BAD_LENGTH` | `0xFD` | Forkert længde, retry/recovery på write. |
| `BAD_CRC` | `0xFB` | CRC fejl, retry/recovery. |
| `TIMEOUT` | `0xFA` | Bootloader timeout, retry/recovery. |

Bemærk at disse statusværdier er dem den mobile implementation håndterer. De kan afvige fra ældre dokumentation der beskriver statuskoder med andre numeriske værdier.

## Firmware discovery og update-plan

Når Firmware Update-skærmen åbnes:

1. Appen kræver BLE connection.
2. `detectTarget()` låser BLE-kommandokanalen.
3. Appen slår CAN passthrough til.
4. Appen sender info broadcast på `0x18EAFFFE`.
5. Appen lytter efter CAN frames med serial, partnumber og firmware versioner.
6. Appen spørger hver funden unit efter bridge firmware versioner:
   - B1: block `0`, id `223`
   - B2: block `9`, id `0`
   - B3: block `9`, id `1`
   - B4: block `9`, id `2`
7. Appen henter firmware catalog fra backend for target partnumber.
8. Appen bygger update-plan per bridge/modul.

Update-planen er produktbaseret: CB/Battery viser kun bridge 1, mens CL/LPS viser bridge 1-4. Hvis en CL/LPS bridge ikke svarer på version request, vises den som `Cannot update` i stedet for at blive skjult. For moduler med en kendt aktuel firmwareversion vælger appen nyeste compatible firmware i samme hardware/family-linje som den installerede version. Det forhindrer automatisk hop mellem inkompatible versionfamilier.

## Selve update-flowet

`runFirmwareUpdate()` udfører denne rækkefølge:

1. Valider BLE connection og API config.
2. Lås BLE command channel, så Dashboard/Settings ikke sender kommandoer under update.
3. Attach CAN notification listener.
4. Resolve CAN ID, partnumber, serial og bridge ID.
5. Slå CAN passthrough til.
6. Scan kort efter eksisterende bootloader heartbeat.
7. Hvis target ikke allerede er i bootloader:
   - send enter boot mode `0x01`
   - send application reset `0x06`
   - vent på bootloader heartbeat
   - retry boot-entry op til 2 forsøg
8. Hvis bootloader melder NO_SERIAL, kopieres serial med status/control action `0x01`.
9. Start keepalive, som ACK'er bootloader heartbeats.
10. Hent firmware catalog og vælg firmwareversion.
11. Åbn ønsket bridge med status/control action `0x02`.
12. Download Intel HEX firmware fra backend.
13. Hent bootloader-info med `READ_VERSION`.
14. Parse HEX til raw image indenfor bootloaderens flashgrænser.
15. Beregn antal erase pages og send `ERASE_FLASH`.
16. Pak image i `WRITE_FLASH`-pakker.
17. Upload alle write-pakker.
18. Vent kort efter sidste write.
19. Send `RESET_DEVICE`.
20. Send bootloader exit/close bridge.
21. Hvis bridge 3/display fik explicit reset-status `0xFA`, send application reset `0x06`.
22. Slå CAN passthrough fra, detach listeners og frigiv BLE lock i `finally`.

`finally`-delen er vigtig: hvis update fejler midtvejs, prøver appen stadig at lukke bootloader bridge, slå passthrough fra og frigive BLE lock.

## Write packet format

Appen skriver firmware i blokke på op til 240 data bytes. Hver write packet er 16 bytes header plus data.

Headeren er:

| Offset | Værdi | Beskrivelse |
| ---: | --- | --- |
| 0 | `0xAF` | Start byte |
| 1 | `0x02` | `WRITE_FLASH` |
| 2-3 | Aligned data length | Little endian |
| 4-7 | `55 00 AA 00` | Unlock sequence |
| 8-11 | Flash address | Little endian |
| 12-13 | `00 00` | Reserved |
| 14-15 | CRC16 | Beregnes efter bytes 14-15 er nulstillet |
| 16+ | Firmware data | Fyldes med `0xFF` til aligned længde |

Et write packet på 256 bytes bliver fragmenteret til 37 CAN frames, fordi hver CAN frame kun bærer 7 bootloader payload bytes. Derfor er BLE batching afgørende for hastighed.

## Write-rækkefølge

Appen følger C#-programmets write-rækkefølge:

```text
Packet 1, 2, 3, ... N-1, derefter Packet 0
```

Packet 0 sendes altså sidst. Det matcher desktop-uploaderens `WriteData`/`WriteDataLast` flow.

## Retry og recovery

Write er det mest sårbare punkt, fordi data går React Native -> BLE -> ESP32 -> CAN -> bootloader.

Appens nuværende strategi:

- `WRITE_FLASH` timeout per command er 900 ms.
- Hver write command kan retryes internt op til 5 gange.
- Efter `OK` på write venter appen 5 ms.
- Ved block-level fejl (`timeout`, `0xFA`, `BAD_LENGTH`, `0xFD`) kan appen åbne bridge igen og prøve blokken igen.
- Block-level recovery prøves op til 3 gange.
- Hvis CAN-frame batching giver timeout, slås batching fra og appen fortsætter med single-frame BLE writes.

Små kommandoer som `READ_VERSION`, `ERASE_FLASH` og `RESET_DEVICE` sendes som reliable BLE writes. Store write payloads sendes streamed/non-reliable for hastighed.

## Reset og cleanup

Efter sidste write gør appen følgende:

1. Venter 750 ms før reset. Det giver target tid til at afslutte sidste flash-write.
2. Sender `RESET_DEVICE` (`0x09`) med op til 5 retries og 4000 ms timeout.
3. Hvis reset ikke svarer, kan det betyde at target allerede er genstartet. Det accepteres.
4. Hvis reset svarer med explicit `0xFA`, fortsætter appen med bridge exit, men markerer at reset muligvis ikke lykkedes.
5. Appen sender exit/close bridge tre gange:

```text
CAN ID: 0x19FF20<unitId>
Data: serial[5], productId, bridgeId=0, action=0x04
```

6. Appen venter 1000 ms efter exit.
7. For display bridge (`bridgeId === 3`) sendes application reset `0x06`, hvis reset endte i explicit `0xFA`.

Display-reset er med fordi en display-update ellers kan ende med korrekt bootload, men uden grafik indtil power-cycle.

## Progress i UI

Servicen kalder `onProgress` med både procent og tekst:

```js
{ progress: number, message: string }
```

Firmware Update-skærmen bruger teksten `Uploading block X/Y` til at vise package progress. Det er bevidst holdt uden logvindue; brugeren ser kun update-plan, progressbar og resultat.

## Vigtige fejlsymptomer

| Symptom | Typisk årsag | Håndtering |
| --- | --- | --- |
| `BLE command channel is busy` | Dashboard/Settings eller anden update holder lock | Vent eller afbryd anden operation. |
| `No bootloader heartbeat` | Target kom ikke i bootloader, forkert CAN ID eller serial mismatch | Appen retryer boot-entry og rediscovery. |
| `Bridge open timeout` | Forkert bridge ID, bridge svarer ikke eller status viser anden bridge | Appen viser last seen bridge i fejltekst. |
| `BAD_CRC` / `0xFB` | CRC eller transportfejl under write | Retry/recovery. |
| `BAD_LENGTH` / `0xFD` | Trunkeret write-payload | Retry/recovery og evt. bridge reopen. |
| `0xFA` under write | Bootloader timeout, ofte transport/trunkering | Retry/recovery. |
| Display viser ikke grafik efter update | Reset lykkedes ikke helt efter bridge 3 update | Appen sender extra application reset ved `0xFA` reset-status. |

## Kodeplacering

| Fil | Ansvar |
| --- | --- |
| `src/screens/FirmwareUpdateScreen.js` | UI, targetvisning, module labels, start/cancel, progress/result. |
| `src/services/firmwareUpdateService.js` | Discovery, update-plan, bootloader state machine, CAN/BLE transport, retry og cleanup. |
| `src/utils/protocol.js` | BLE command encoders/decoders, inkl. CAN frame og CAN frame batching. |
| ESP32 `09_CAN_HMI` firmware | BLE gateway og CAN forwarding. |

## Ting man ikke bør ændre uden hardwaretest

- Write packet header: `55 00 AA 00`, reserved `00 00`, CRC bytes nulstillet før CRC.
- Write-rækkefølge: packet 0 sidst.
- Heartbeat ACK keepalive under hele sessionen.
- Bridge exit med `bridgeId=0`, action `0x04`, gentaget tre gange.
- Display special-case efter bridge 3 update.
- BLE command lock omkring update-flowet.
- CAN frame batching fallback til single-frame writes.

Disse detaljer er alle kommet fra parity med C#-programmet og hardwaretests.