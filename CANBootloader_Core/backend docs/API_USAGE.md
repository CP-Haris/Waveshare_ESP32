# Firmware Backend API Usage

## Base paths

Direkte paa backend:

- `http://127.0.0.1:5080/api/v1/...`

Via nginx path-prefix:

- `http://127.0.0.1/firmware-api/api/v1/...`

## Authentication

Beskyttede endpoints kraever header:

- `X-Api-Key: <api-key>`

Anonyme endpoints:

- `GET /api/v1/health/live`
- `GET /api/v1/health/ready`

## Endpoints

### Health

- `GET /api/v1/health/live`
- `GET /api/v1/health/ready`

### Firmware

- `GET /api/v1/firmware/catalog?partNumber=<pn>&includePrototype=<true|false>`
- `GET /api/v1/firmware/download?partNumber=<pn>&bridgeId=<id>&version=<optional>`

### Bootloader logs

- `POST /api/v1/bootloader/logs`

## Eksempler

Health (ingen API key):

```bash
curl -s http://127.0.0.1:5080/api/v1/health/live
curl -s http://127.0.0.1:5080/api/v1/health/ready
```

Firmware catalog:

```bash
curl -s -H "X-Api-Key: <api-key>" "http://127.0.0.1:5080/api/v1/firmware/catalog?partNumber=12345&includePrototype=false"
```

Firmware download:

```bash
curl -L -H "X-Api-Key: <api-key>" "http://127.0.0.1:5080/api/v1/firmware/download?partNumber=12345&bridgeId=1" -o firmware.bin
```

Bootloader log upload:

```bash
curl -s -X POST "http://127.0.0.1:5080/api/v1/bootloader/logs" \
  -H "Content-Type: application/json" \
  -H "X-Api-Key: <api-key>" \
  -d '{"status":"Success","message":"Update completed","partNumber":"12345","bridgeId":1}'
```