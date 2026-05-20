# CAN Bootloader Console

Cross-platform console app for scanning CAN devices and updating firmware via bootloader protocol.

## Runtime model

- Firmware source: API only
- Bootloader logs: API only (with local cache retry)
- Local encrypted firmware cache is used for offline resilience

## Requirements

- .NET 8 SDK (for build)
- Serial/CAN adapter
- Network access to firmware API endpoint

## Build

```bash
dotnet build
```

## Run

```bash
dotnet run --project ./app/CANBootloaderConsole.csproj
```

## Main menu

Simple mode:
- 1 Update Firmware
- 2 Reload Current Firmware
- R Reset Firmware Cache
- M Change mode (if enabled)
- Q Quit

Advanced mode:
- 1 Scan CAN Bus for Devices
- 2 Update Firmware
- 3 Update Firmware Cache
- 4 View Cached Firmware
- 5 List COM Ports
- R Reset Firmware Cache
- M Change mode
- Q Quit

## Project structure

- app/src/: app source files
- backend/FirmwareBackendApi/: backend API project
- app/docs/: app documentation
- backend/docs/: backend documentation (deployment and API usage)

## Release

Versioning is derived from git tags (MinVer).

- Tag format: vX.Y.Z
- Build/sign script: app/sign-release.ps1

## Notes

- API configuration is hardcoded in client startup (`app/src/Program.cs`) for deterministic behavior.
- Environment variables are intentionally not used for API base URL, API key, or timeout.
- Startup performs automatic firmware cache sync from API.
- Backend deployment notes are documented in `backend/docs/README.md`.
