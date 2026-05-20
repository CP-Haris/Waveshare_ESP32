# Quick Test Guide - Remote Mac Debugging

## Hurtig validering af setup

Brug placeholders i kommandoer nedenfor:

- `<mac-user>`
- `<mac-host>`

### Test 1: SSH-forbindelse

```powershell
ssh <mac-user>@<mac-host> "echo SSH OK"
```

Forventet resultat: `SSH OK`.

### Test 2: .NET paa Mac

```powershell
ssh <mac-user>@<mac-host> "~/.dotnet/dotnet --version"
```

Forventet resultat: version i 8.0-serien.

### Test 3: vsdbg er installeret

```powershell
ssh <mac-user>@<mac-host> "test -f ~/vsdbg/vsdbg && echo installed || echo missing"
```

Forventet resultat: `installed`.

### Test 4: Build til Mac

```powershell
dotnet build app/CANBootloaderConsole.csproj -r osx-arm64 --configuration Debug
```

Forventet resultat: `Build succeeded`.

### Test 5: Deploy-resultat findes paa Mac

```powershell
ssh <mac-user>@<mac-host> "ls -lh ~/CANBootloader/CANBootloaderConsole"
```

Forventet resultat: fil-info vises.

### Test 6: Koersel paa Mac

```powershell
ssh <mac-user>@<mac-host> "cd ~/CANBootloader && ./CANBootloaderConsole --version"
```

Forventet resultat: banner/version vises.

### Test 7: Debug launch

1. Tryk `F5` i VS Code.
2. Vælg `.NET Core Launch (Remote - Mac)`.
3. Sæt breakpoint i `app/src/Program.cs`.
4. Bekræft at breakpoint rammes.

## Hvis en test fejler

1. Kør tasken `test-connection`.
2. Kør tasken `install-vsdbg-mac`.
3. Kør tasken `build-and-deploy-mac` igen.
4. Se `TROUBLESHOOTING.md`.
