# Remote Mac Debugging Guide

Denne guide beskriver den aktuelle og vedligeholdte måde at debugge CANBootloader på en remote Mac.

## Forudsætninger

- VS Code med C# debugging
- SSH-adgang til Mac
- Task-konfiguration i `../../.vscode/tasks.json`
- Launch-konfiguration i `../../.vscode/launch.json`

## Hurtigstart (anbefalet)

1. Åbn projektet i VS Code.
2. Tryk `F5`.
3. Vælg `.NET Core Launch (Remote - Mac)`.

Konfigurationen kører automatisk tasken `build-and-deploy-mac`, deployer til `~/CANBootloader` og starter derefter debug-session via `vsdbg` over SSH.

## Relevante tasks

- `build-mac`: bygger app til `osx-arm64`.
- `create-remote-dir`: sikrer at `~/CANBootloader` findes.
- `deploy-to-mac`: kopierer build-output til Mac.
- `build-and-deploy-mac`: fuld sekvens for normal remote debug.
- `install-vsdbg-mac`: installerer/opdaterer `vsdbg` på Mac.
- `test-connection`: hurtig SSH/.NET check.

Se samlet oversigt i `TASKS_GUIDE.md`.

## SSH-nøgle (anbefalet)

```powershell
# Generer SSH nøgle hvis nødvendig
ssh-keygen -t rsa -b 4096 -f "$env:USERPROFILE\.ssh\id_rsa"

# Kopier public key til Mac
Get-Content "$env:USERPROFILE\.ssh\id_rsa.pub" | ssh <mac-user>@<mac-host> "mkdir -p ~/.ssh; cat >> ~/.ssh/authorized_keys"
```

## Fejlfinding

1. Bekræft SSH: `ssh <mac-user>@<mac-host> "echo SSH OK"`
2. Bekræft debugger: `ssh <mac-user>@<mac-host> "ls -la ~/vsdbg/vsdbg"`
3. Bekræft remote app-path: `ssh <mac-user>@<mac-host> "ls -la ~/CANBootloader/CANBootloaderConsole"`
4. Ved breakpoint-problemer: check `sourceFileMap` i `../../.vscode/launch.json`.

## Vigtigt

- Undlad at lægge passwords eller andre secrets i dokumentation.
- Remote debugging har begrænset understøttelse af interaktiv console-input. Brug SSH + direkte kørsel til manuel interaktiv test.

## Relaterede dokumenter

- `REMOTE_DEBUG_SETUP.md`
- `QUICK_TEST.md`
- `TROUBLESHOOTING.md`
