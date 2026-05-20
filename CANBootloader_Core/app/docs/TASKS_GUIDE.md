# VS Code Tasks Oversigt

Dette dokument afspejler task-definitionerne i `../../.vscode/tasks.json`.

## App tasks

### `build`

Bygger app-projektet:

```powershell
dotnet build ${workspaceFolder}/app/CANBootloaderConsole.csproj
```

### `build-mac`

Bygger app til macOS ARM64:

```powershell
dotnet build ${workspaceFolder}/app/CANBootloaderConsole.csproj -r osx-arm64
```

### `create-remote-dir`

Sikrer remote mappe findes:

```powershell
ssh <mac-user>@<mac-host> "mkdir -p ~/CANBootloader"
```

### `deploy-to-mac`

Kopierer output til Mac:

```powershell
scp -r ${workspaceFolder}/app/bin/Debug/net8.0/osx-arm64/* <mac-user>@<mac-host>:~/CANBootloader/
```

### `build-and-deploy-mac`

Sekvens-task:

1. `build-mac`
2. `create-remote-dir`
3. `deploy-to-mac`

### `install-vsdbg-mac`

Installerer/opdaterer debugger paa Mac:

```powershell
ssh <mac-user>@<mac-host> "curl -sSL https://aka.ms/getvsdbgsh | bash /dev/stdin -v latest -l ~/vsdbg"
```

### `test-connection`

Tester SSH og .NET paa Mac.

### `ssh-to-mac`

Aabner SSH session.

### `run-on-mac`

Koerer app direkte paa Mac:

```powershell
ssh <mac-user>@<mac-host> "cd ~/CANBootloader && ./CANBootloaderConsole"
```

## Backend tasks

### `publish-backend-linux-selfcontained`

Publicerer backend som self-contained Linux x64 build til:

`backend/artifacts/firmware-backend-linux-selfcontained`

### `package-backend-linux-selfcontained`

Pakker publish-output i:

`backend/artifacts/firmware-backend-linux-selfcontained.tar.gz`

### `publish-backend-linux-selfcontained-refresh`
### `publish-backend-linux-selfcontained-proxyfix`
### `package-backend-linux-selfcontained-proxyfix`

Varianter af publish/package med samme outputsti.

## Anbefalet workflow

1. App remote debug: tryk `F5` med `.NET Core Launch (Remote - Mac)`.
2. App manuel test: `build-and-deploy-mac` efterfulgt af `run-on-mac`.
3. Backend deploy-artifact: `publish-backend-linux-selfcontained` efterfulgt af `package-backend-linux-selfcontained`.

## Fejlfinding

1. Kør `test-connection` ved SSH-problemer.
2. Kør `install-vsdbg-mac` ved debugger-problemer.
3. Se `TROUBLESHOOTING.md` for dybere diagnose.
