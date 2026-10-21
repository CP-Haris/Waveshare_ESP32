# LPS2 Display: indstillinger og min/maks fra appen

**Til:** udvikleren af LPS2-displaysoftwaren (`R027_LPS2_Display_APPLICATION`)
**Fra:** Haris Hujic, R&D
**Fil, der skal ændres:** `src/UART_App.c`, funktionen `COM_GetRXData()`, BLE-delen (`#ifdef BLUETOOTH_CODE`, `if(Buffer->Uart_Module == UART_BLE)`)

## Problemet

Den nye Clayton Power-app kan læse alle LPS-indstillinger over den indbyggede Bluetooth, men den kan ikke **gemme** dem. Når appen sender en ny værdi, vises den kort på displayet og forsvinder igen. Kontrolboardet får den aldrig.

## Årsag

Displayet bruger selv `0x50` (SET_VAL), når det gemmer en indstilling på kontrolboardet. Se `Menu_Setting.c` linje 256:

```c
if(Item->ST_Value->Data->Flag.Owned_CTRL)
    Values_Send_ByValue(SET_VAL, (Value*)Item->ST_Value->Data, UART_CTRL);
```

Fra BLE er der i dag ingen vej, der ender som en `0x50` til kontrolboardet:

| Appen sender | Hvad displayet gør i dag | Resultat |
|---|---|---|
| `20 bb ii v0 v1 v2 v3` | `case 0x20` gemmer værdien i RAM og sender den **uændret som 0x20** til kontrolboardet (`Access_CTRL`) | Kontrolboardet opfatter 0x20 som læs/svar og gemmer intet. Displayet viser den nye værdi, indtil næste polling overskriver den med kontrolboardets gamle værdi. Derfor ses værdien kortvarigt. |
| `50 bb ii v0 v1 v2 v3` | `case 0x50` gemmer værdien i displayets RAM og stopper der. Casen er lavet til BLE-modulets egne værdier (passkey, MAC) | Når aldrig kontrolboardet. |

## Rettelse 1: gem indstilling (lavet)

### Ønsket ændring

`case 0x50` fra BLE skal skelne mellem, hvem der ejer værdien:

- **`Owned_BLE`** (blok 200: BLE-status, passkey, MAC, whitelist): uændret adfærd. Det er BLE-modulet selv, der melder sine værdier.
- **`Owned_CTRL` med `Access_BLE`** (alle LPS-indstillinger, fx blok 1, 3, 30, 40, 50, 60, 70): gem i RAM, og send videre til kontrolboardet med `SET_VAL`, præcis som displayets egen indstillingsmenu gør.
- Alt andet ignoreres.

Erstat den nuværende `case 0x50` i BLE-delen med:

```c
//Set value: BLE module's own values (passkey/MAC) or an LPS setting from the app
case 0x50:
{
    Value *Val = Values_GetValue(Buffer->Msg_Buffer[1], Buffer->Msg_Buffer[2]);

    if((Val != NULL) && (Buffer->Msg_Counter == 7))
    {
        long NewValue = (long)(Buffer->Msg_Buffer[6] << 24) | (long)(Buffer->Msg_Buffer[5] << 16)
                      | (long)(Buffer->Msg_Buffer[4] << 8)  | (long)Buffer->Msg_Buffer[3];

        //Value owned by the BLE module itself (unchanged behaviour)
        if(Val->Flag.Owned_BLE)
        {
            Val->NumValue = NewValue;
            Val->Flag.NumValue_Received = true;

            if(Val->ID == 102)
                BUZ_SetMelody(BUZ_BIP);
        }
        //LPS setting written from the app: store and forward to the control board,
        //the same way Menu_Setting.c saves a setting
        else if(Val->Flag.Owned_CTRL && Val->Flag.Access_BLE)
        {
            Val->NumValue = NewValue;
            Val->Flag.NumValue_Received = true;
            GlobalState.Update_Graphic = true;
            Values_Send_ByValue(SET_VAL, Val, UART_CTRL);
        }
    }
}
break;
```

**Bemærk:** den gamle `case 0x50` tjekkede ikke `Msg_Counter == 7`. Det gør den nye, så korte eller ødelagte frames ikke skriver tilfældige værdier.

### Anbefalet ekstra rettelse (valgfri)

I `case 0x20` fra BLE gemmes en `0x20`-værdi på 7 bytes i RAM uanset ejer, og sendes videre til kontrolboardet. Det er det, der giver den forvirrende "værdi vises kort og forsvinder"-effekt. En `0x20` med værdi fra BLE er i praksis kun BLE-modulets svar på displayets egne forespørgsler (`Owned_BLE`). Overvej derfor kun at acceptere den for `Owned_BLE`-værdier:

```c
if(Buffer->Msg_Counter == 7){
    if(!Val->Flag.Owned_BLE)
        break;   // Settings from the app must use 0x50 (SET_VAL)
    ...
```

Rettelsen er ikke nødvendig for, at appen virker. Den gør bare, at en forkert kommando ikke længere ser ud, som om den virkede.

## Rettelse 2: min/maks når aldrig appen (lavet)

**Status før rettelsen:** rettelse 1 (gem indstilling) er lavet og virker. Appen kan nu gemme, men får ingen min/maks, så editoren kan ikke vise grænserne.

**Observeret** (app-log, Inverter Cutoff = blok 50, id 0):

```
→ 22 32 00                   appen beder om min/maks
                             intet 22-svar kommer tilbage (heller ikke efter flere sekunder)
→ 20 32 00                   appen læser værdien
← 20 32 00 E9 11 00 00       svar kommer straks
```

**Årsag:** BLE-delen har ingen `case 0x22`. Forespørgslen havner i `default` og sendes blindt videre til kontrolboardet, men der kommer intet `0x22`-svar retur til BLE. Displayet har ofte min/maks liggende allerede (`NumMinMax_Received`), fordi menuen henter dem én gang (`Menu_Setting.c` linje 51, `UpdateIfSet = false`). Nogle værdier har dem endda faste i `Values.c` (fx 1:1, 30:1, 7:0), og dem kender kontrolboardet måske slet ikke.

**Ønsket ændring:** håndtér `0x22` fra BLE ligesom `0x20`. Svar fra displayets cache, når min/maks kendes. Ellers bed kontrolboardet om dem, og så sender den eksisterende `case 0x22` på CTRL-siden svaret videre til BLE.

Tilføj i BLE-delens `switch` (ved siden af `case 0x50`):

```c
//Min/max request from the app: answer from the display's cache when known,
//otherwise ask the control board (its 0x22 reply is forwarded to BLE in the
//UART_CTRL part, case 0x22)
case 0x22:
{
    Value *Val = Values_GetValue(Buffer->Msg_Buffer[1], Buffer->Msg_Buffer[2]);

    if((Val != NULL) && (Buffer->Msg_Counter == 3) && Val->Flag.Access_BLE)
    {
        if(Val->Flag.NumMinMax_Received)
        {
            unsigned char Data[11];
            Data[0]  = GET_MIN_MAX;
            Data[1]  = Val->Block;
            Data[2]  = Val->ID;
            Data[3]  = (unsigned char)(Val->NumMin);
            Data[4]  = (unsigned char)(Val->NumMin >> 8);
            Data[5]  = (unsigned char)(Val->NumMin >> 16);
            Data[6]  = (unsigned char)(Val->NumMin >> 24);
            Data[7]  = (unsigned char)(Val->NumMax);
            Data[8]  = (unsigned char)(Val->NumMax >> 8);
            Data[9]  = (unsigned char)(Val->NumMax >> 16);
            Data[10] = (unsigned char)(Val->NumMax >> 24);
            SendMSG(Data, sizeof(Data), UART_BLE);
        }
        else if(Val->Flag.Owned_CTRL)
        {
            Values_RequestByValue(Val, true, GET_MIN_MAX);
        }
    }
}
break;
```

**Test:**

| | Frame på UART2 (hex) |
|---|---|
| App beder om min/maks for 50:0 | `01 22 32 00 51 8B 04` |
| Forventet svar | `01 22 32 00 <min 4 bytes> <max 4 bytes> <CRCL> <CRCH> 04` |

1. Kontrollér først på UART2, at `22 32 00` faktisk når displayet. Hvis ikke, filtrerer BLE-modulet kommandoen fra, og så skal rettelsen laves i BLE-modulets firmware.
2. Kontrollér, at svaret kommer tilbage på UART2, både når displayet har min/maks i cache (åbn indstillingen i displayets egen menu først) og når det ikke har (lige efter opstart).
3. Kontrollér, at min/maks svarer til det, displayets egen menu tillader.

## Protokollen efter rettelsen (app → LPS2 via BLE)

| Handling | Payload | Svar |
|---|---|---|
| Læs indstilling | `20 bb ii` | `20 bb ii v0 v1 v2 v3` |
| Læs min/maks | `22 bb ii` | `22 bb ii min32 max32`, fra displayets cache eller fra kontrolboardet (rettelse 2) |
| **Gem indstilling** | **`50 bb ii v0 v1 v2 v3`** | Appen læser værdien igen med `20 bb ii` for at bekræfte |

Værdier er Q16.16, little endian, ligesom i resten af protokollen.

## Test

Eksempel: sæt *Inverter Cutoff* (blok 50, id 0) til 85 % (0,85 × 65536 = `0x0000D99A`).

| | Frame på UART2 (hex) |
|---|---|
| Gem 85 % | `01 50 32 00 9A D9 00 00 6A 1B 04` |
| Læs igen | `01 20 32 00 31 E5 04` |

1. Send "Gem" fra appen, eller direkte på UART2 med en USB-UART.
2. Kontrollér på UART1, at displayet sender `50 32 00 9A D9 00 00` (indrammet) til kontrolboardet.
3. Send "Læs igen". Svaret skal være `20 32 00 9A D9 00 00`, og værdien skal stå i displayets egen menu efter genstart.
4. Gentag med en værdi uden for min/maks og kontrollér, at kontrolboardet afviser eller begrænser den.
5. Kontrollér, at BLE-parring (passkey på displayet) stadig virker. Det er `Owned_BLE`-grenen.

## Sikkerhed

Enhver telefon, der er parret (bonded) med LPS'en, kan efter rettelsen ændre alle indstillinger, der har `Access_BLE`. Det er tilsigtet og svarer til displayets egen menu. Parring kræver den passkey, der vises på displayet.

Allerede i dag kan en parret telefon overskrive `Owned_BLE`-værdier i displayets RAM via `0x50`, fordi displayet ikke kan se, om en frame på UART2 kommer fra BLE-modulet eller fra telefonen. Det ændrer rettelsen ikke på. Skal det lukkes, kræver det, at BLE-modulet markerer sine egne frames.

---
*Analysen er lavet ud fra `UART_App.c`, `Values.c`, `Menu_Setting.c` og protokoldokumentet i `firmware/Documentation`. Den fulde BLE-protokol er beskrevet i `Docs/LPS2-BLE-Protocol.md`.*
