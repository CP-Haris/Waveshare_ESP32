# Setup For Remote Mac Debugging

Denne guide beskriver et rent setup uden hardkodede credentials.

## 1. Test SSH-forbindelse

```powershell
ssh <mac-user>@<mac-host>
```

Hvis det er første gang, accepter host key.

## 2. Opsaet SSH-noegle (anbefalet)

```powershell
# Generer noegle hvis du ikke har en
ssh-keygen -t rsa -b 4096 -f "$env:USERPROFILE\.ssh\id_rsa"

# Kopier public key til Mac
Get-Content "$env:USERPROFILE\.ssh\id_rsa.pub" | ssh <mac-user>@<mac-host> "mkdir -p ~/.ssh && cat >> ~/.ssh/authorized_keys"
```

## 3. Installer VS debugger paa Mac

Kør tasken `install-vsdbg-mac` i VS Code, eller manuelt:

```bash
ssh <mac-user>@<mac-host>
curl -sSL https://aka.ms/getvsdbgsh | bash /dev/stdin -v latest -l ~/vsdbg
```

## 4. Build og deploy

Kør tasken `build-and-deploy-mac`.

Task-sekvens:

1. `build-mac`
2. `create-remote-dir`
3. `deploy-to-mac`

## 5. Start debugging

1. Tryk `F5`.
2. Vælg `.NET Core Launch (Remote - Mac)`.

## Hurtig validering

```powershell
# SSH
ssh <mac-user>@<mac-host> "echo SSH OK"

# Byg
dotnet build app/CANBootloaderConsole.csproj -r osx-arm64 --configuration Debug

# Deploy
scp -r "app/bin/Debug/net8.0/osx-arm64/*" <mac-user>@<mac-host>:~/CANBootloader/

# Test koersel
ssh <mac-user>@<mac-host> "cd ~/CANBootloader && chmod +x CANBootloaderConsole && ./CANBootloaderConsole --version"
```

## Relateret

- `MAC_DEBUG_README.md`
- `QUICK_TEST.md`
- `TROUBLESHOOTING.md`
