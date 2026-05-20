# Remote Mac Debugging - Setup Checkliste

Dette dokument er en vedligeholdelsesvenlig checkliste i stedet for et statisk setup-snapshot.

## 1. VS Code konfiguration

- [ ] `../../.vscode/launch.json` findes og har `.NET Core Launch (Remote - Mac)`
- [ ] `../../.vscode/tasks.json` indeholder `build-and-deploy-mac`
- [ ] `../../.vscode/settings.json` er gyldig

## 2. Mac miljoe

- [ ] SSH virker: `ssh <mac-user>@<mac-host> "echo SSH OK"`
- [ ] .NET 8 findes: `ssh <mac-user>@<mac-host> "~/.dotnet/dotnet --version"`
- [ ] `vsdbg` findes: `ssh <mac-user>@<mac-host> "ls ~/vsdbg/vsdbg"`
- [ ] App-path findes: `ssh <mac-user>@<mac-host> "ls ~/CANBootloader/CANBootloaderConsole"`

## 3. Build/deploy pipeline

- [ ] `build-mac` virker
- [ ] `build-and-deploy-mac` virker
- [ ] `F5` starter remote debug uden fejl

## 4. Funktionel test

- [ ] Breakpoint i `app/src/Program.cs` rammes under remote debug
- [ ] Non-interaktiv koersel virker: `./CANBootloaderConsole --version`

## 5. Sikkerhed og hygiene

- [ ] Ingen passwords/API-noegler i dokumentation
- [ ] SSH noegle-login er sat op (anbefalet)

## Hurtig references

- Setup: `REMOTE_DEBUG_SETUP.md`
- Quick test: `QUICK_TEST.md`
- Fejlfinding: `TROUBLESHOOTING.md`
