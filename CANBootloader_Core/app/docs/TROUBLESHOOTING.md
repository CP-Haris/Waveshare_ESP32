# Troubleshooting - Remote Mac Debugging

Denne guide dækker de mest almindelige fejl ved remote debugging af appen.

## 1. Hurtig diagnose

Kør dette i PowerShell:

```powershell
Write-Host "=== Remote Debug Diagnostic ===" -ForegroundColor Green

Write-Host "1) SSH" -ForegroundColor Yellow
ssh <mac-user>@<mac-host> "echo SSH OK" 2>&1

Write-Host "2) .NET" -ForegroundColor Yellow
ssh <mac-user>@<mac-host> "~/.dotnet/dotnet --version" 2>&1

Write-Host "3) vsdbg" -ForegroundColor Yellow
ssh <mac-user>@<mac-host> "test -f ~/vsdbg/vsdbg && echo installed || echo missing" 2>&1

Write-Host "4) Remote app path" -ForegroundColor Yellow
ssh <mac-user>@<mac-host> "ls -la ~/CANBootloader | head -10" 2>&1

Write-Host "5) Local mac build output" -ForegroundColor Yellow
if (Test-Path "app/bin/Debug/net8.0/osx-arm64/CANBootloaderConsole") {
    Write-Host "Local build output found" -ForegroundColor Green
} else {
    Write-Host "Missing local output. Run build-mac" -ForegroundColor Red
}

Write-Host "=== Done ===" -ForegroundColor Green
```

## 2. SSH-forbindelse fejler

Symptomer:

- Timeout eller host unreachable
- Tasken `test-connection` fejler

Loesning:

1. Bekraeft netvaerk: `ping <mac-host>`
2. Bekraeft port 22: `Test-NetConnection -ComputerName <mac-host> -Port 22`
3. Koer verbose: `ssh -v <mac-user>@<mac-host>`
4. Bekraeft Remote Login paa Mac.

## 3. vsdbg mangler

Symptomer:

- F5 starter ikke debugger
- Fejl om manglende `~/vsdbg/vsdbg`

Loesning:

1. Koer task `install-vsdbg-mac`
2. Verificer: `ssh <mac-user>@<mac-host> "ls -la ~/vsdbg/vsdbg"`

## 4. Breakpoints bliver ikke ramt

Symptomer:

- Graa/hule breakpoints
- "Breakpoint will not currently be hit"

Loesning:

1. Byg i Debug:

```powershell
dotnet build app/CANBootloaderConsole.csproj -r osx-arm64 --configuration Debug
```

2. Redeploy:

```powershell
scp -r "app/bin/Debug/net8.0/osx-arm64/*" <mac-user>@<mac-host>:~/CANBootloader/
```

3. Check `sourceFileMap` i `../../.vscode/launch.json`.

## 5. Permission denied paa Mac

Symptom:

- `./CANBootloaderConsole: Permission denied`

Loesning:

```powershell
ssh <mac-user>@<mac-host> "chmod +x ~/CANBootloader/CANBootloaderConsole"
```

## 6. Deploy er langsom eller timeout

Loesning:

1. Bekraeft diskplads: `ssh <mac-user>@<mac-host> "df -h ~"`
2. Bekraeft mappeindhold: `ssh <mac-user>@<mac-host> "ls -la ~/CANBootloader"`
3. Koer `build-and-deploy-mac` igen.

## 7. Console input virker ikke under remote debug

Det er forventet, at interaktiv input kan vaere begranset under remote debug.

Brug i stedet direkte SSH-korsel:

```powershell
ssh <mac-user>@<mac-host>
cd ~/CANBootloader
./CANBootloaderConsole
```

## 8. Host key mismatch

```powershell
ssh-keygen -R <mac-host>
ssh <mac-user>@<mac-host>
```

## 9. Sidste udvej

1. Koer `build-mac`
2. Koer `build-and-deploy-mac`
3. Koer `install-vsdbg-mac`
4. Start debugging med `F5`

Hvis det stadig fejler, indsaml:

- output fra diagnose-scriptet
- output fra `ssh -vvv <mac-user>@<mac-host>`
- debug-output fra VS Code