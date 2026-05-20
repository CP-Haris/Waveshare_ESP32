# Firmware Backend Deployment

## Scope

Deployment-scope paa serveren er bevidst begraenset til:

- `/opt/firmware-backend`
- `/etc/systemd/system/firmware-backend.service`
- `/etc/firmware-backend.env`

Undgaa at aendre andre services, sites eller systemkonfigurationer paa samme host.

## Build og package (lokalt)

Fra workspace root:

```powershell
dotnet publish .\backend\FirmwareBackendApi\FirmwareBackendApi.csproj -c Release -r linux-x64 --self-contained true -o .\backend\artifacts\firmware-backend-linux-selfcontained
```

Pak artefakt:

```powershell
tar -czf .\backend\artifacts\firmware-backend-linux-selfcontained.tar.gz -C .\backend\artifacts\firmware-backend-linux-selfcontained .
```

Alternativt via tasks:

1. `publish-backend-linux-selfcontained`
2. `package-backend-linux-selfcontained`

## Server-side deploy

Deploy-script:

- `backend/FirmwareBackendApi/deploy/deploy-backend.sh`

Scriptet:

1. udpaker tarball til `/opt/firmware-backend`
2. saetter executable bit paa `FirmwareBackendApi`
3. kopierer servicefil til `/etc/systemd/system/firmware-backend.service`
4. restarter systemd-servicen `firmware-backend`

## Miljoevariabler

Eksempelfil:

- `backend/FirmwareBackendApi/deploy/firmware-backend.env.example`

Noegler:

- `ASPNETCORE_URLS=http://127.0.0.1:5080`
- `Auth__ApiKeys__0=<strong-api-key>`
- `ConnectionStrings__FirmwareDb=<mysql-connection-string>`

## Verifikation efter deploy

Koer begge health checks:

```bash
curl -fsS http://127.0.0.1:5080/api/v1/health/ready
curl -fsS http://127.0.0.1/firmware-api/api/v1/health/ready
```

Begge endpoints skal returnere status `ready` (eller passende statuskode ved DB-problemer).

## Fejlfinding

- `systemctl status firmware-backend --no-pager --full`
- `journalctl -u firmware-backend -n 200 --no-pager`
- Bekraeft env-fil findes og laeses af servicefilen (`EnvironmentFile=/etc/firmware-backend.env`).