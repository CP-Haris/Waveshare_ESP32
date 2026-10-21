# TODO / Nice to have

## Fejl at undersøge
- [ ] **Enheden på CAN-adresse 0x03 svarer ikke på identifikation** (fundet
      2026-10-21 efter flash af `ebe62c9`). Displayet sender `0x18EA03FE`
      hvert 5. s, men der kommer ingen `0x19FFxx`-svar — og heller intet
      `0x19EF`-svar på en CAN_Extra GET (block 60, id 2), også når den sendes
      direkte fra PC via USB-modemet. Bussen er sund (heartbeat: `state=1
      tec=0 rec=0 txq=0`), så frames ACK'es. Enheden sender kun sine
      broadcasts (`18FF00`…`18FF22`). Følge: intet navn/type og ingen
      indstillinger for den enhed. Næste skridt: afklar om den har svaret
      før (evt. flash `9d643a4` og sammenlign), og hvilken enhed/firmware
      det er.

## Tid / RTC
- [x] Tidssynkronisering fra telefon-app via BLE (BLE_CMD_SYNC_TIME, når uret
      ikke er stillet)
- [ ] Tidssynkronisering fra CAN-bus (hvis LPS/BMS udstiller tid)
- [ ] Tidszone/sommertid-håndtering (RTC'en kører naiv lokaltid i dag)
- [ ] Dato i headeren eller på en info-side (kun HH:MM vises nu)

## Indstillinger
- [ ] Flere lokale display-indstillinger under General (lysstyrke,
      sleep-timeout, sprog?)

## Diverse
- [ ] Genmål standby-strøm efter TinyUSB soft-detach-ændringen
      (heartbeat-loggens lightsleep-% giver en hurtig indikation)
- [x] Migrér `esp_lcd_touch_get_coordinates` -> `esp_lcd_touch_get_data`
- [ ] Dokumentér USB CAN-modemet i Docs/ESP32-Firmware.md
      (protokol, porte, DTR-krav, BT/TS-kommandoer, flash.ps1)
