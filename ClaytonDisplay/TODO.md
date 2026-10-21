# TODO / Nice to have

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
