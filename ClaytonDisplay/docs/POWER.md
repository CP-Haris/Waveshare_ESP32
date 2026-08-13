# Standby power on the ESP32-S3-Touch-LCD-5

Record of the standby-current investigation: what was measured, what was wrong,
what was changed, and what is left. Board is the Waveshare ESP32-S3-LCD-5
(WROOM-1 + CH422G + GT911 + TJA1051 CAN + SP3485 RS485 + opto DI/DO), ESP-IDF
v6.0.

## Result

| Stage | Standby current @12 V |
|---|---|
| Before | 23 mA |
| After the RS485 mistake (see below) | 29 mA |
| TWAI teardown + LVGL tick stopped | 19–20 mA (~80 % of the time; brief dips to 5–10 mA) |
| CANRX wake source removed | **8–9 mA** |

Target was 5 mA. The remaining gap is dominated by hardware — see
[What is left](#what-is-left).

## The three things that blocked light sleep

`pwr_display_off()` had always called `esp_pm_configure(..., light_sleep_enable
= true)`, but the chip never actually slept. Three independent causes, each
sufficient on its own, stacked behind one another.

### 1. The TWAI driver holds a PM lock for its whole lifetime

On ESP32-S3 `TWAI_CLK_SRC_DEFAULT` is `SOC_MOD_CLK_APB`, so the driver creates
an `ESP_PM_APB_FREQ_MAX` lock and acquires it in `twai_driver_install()` —
IDF's own comment reads *"Acquire pm_lock during the whole driver lifetime"*
(`components/driver/twai/twai.c`). In `esp_pm/pm_impl.c` any held
`APB_FREQ_MAX` resolves to `PM_MODE_APB_MAX`, never `PM_MODE_LIGHT_SLEEP`.

**`twai_stop()` does not release it. Only `twai_driver_uninstall()` does.**

Fix: the bus lifecycle moved out of `app_main()` into `can_hmi_bus_start()` /
`can_bus_stop()`; standby tears the driver down completely and brings it back
for each listening window.

### 2. LVGL's 2 ms tick timer runs through standby

`lvgl_port.c` starts an `esp_timer` with a 2 ms period for `lv_tick_inc()`.
`lvgl_port_suspend()` suspended the LVGL *task* but left the timer running, and
the handle was a local variable so it could not be stopped.

A 500 Hz wake source makes automatic light sleep pointless: tickless idle can
never sleep longer than 2 ms, while `CONFIG_ESP_SLEEP_WAIT_FLASH_READY_EXTRA_DELAY`
alone is 2000 µs per wake. The power manager simply declines.

Fix: the handle is now at file scope; suspend stops the timer, resume restarts
it. Nothing consumes `lv_tick` while the task is suspended, and wake does a full
redraw anyway.

### 3. CANRX as a light-sleep wake source, on a bus that is never quiet

`gpio_wakeup_enable(IO16, GPIO_INTR_LOW_LEVEL)` was armed so a dominant bit
could wake the CPU. But the LPS/BMS units keep broadcasting their *idle* status
while the display sleeps, so the level-triggered wake fired continuously and
shredded each 250 ms sleep slice into fragments — each paying the ~2 ms wake
overhead. Measured: awake ~80 % of the time.

Fix: disabled (`PWR_CAN_GPIO_WAKE 0`). The bus is sampled by a periodic window
instead. The code is kept behind the flag in case a future installation has a
genuinely quiet bus.

Note on the diagnostic that misled us: the `canwake` counter only increments
when a sleep *ended* on GPIO. While the chip never slept it read 0, which looked
like "the wake never fires" when it actually meant "there is no sleep to end".

## Standby architecture as it stands

Deep idle (no BLE connection):

- TWAI uninstalled — no PM lock, light sleep permitted.
- `vTaskDelay(PWR_SLEEP_POLL_MS)` = 250 ms slices; this is where the chip
  light-sleeps. Also sets touch response latency.
- Touch polled over I2C on each wake (GT911 stays powered — it must keep
  scanning for the poll to see anything).
- Every `PWR_CAN_WINDOW_PERIOD_MS` (3 s) the bus is sampled: install TWAI,
  listen `PWR_CAN_WINDOW_MS` (150 ms), decode, uninstall. ~5 % duty cycle.
- Display only wakes if `lps_wants_wake()` — a unit with ≥10 messages received
  reports a running state. Bus traffic alone is deliberately not enough.

BLE connected: the bus stays up and light sleep is not attempted. A phone
session is short and interactive.

BLE advertising in standby is 800–1000 ms, connectable (`BLE_GAP_CONN_MODE_UND`)
— unchanged, it already met the requirement.

### Tuning knobs

| Constant | Now | Effect |
|---|---|---|
| `PWR_SLEEP_POLL_MS` | 250 | Touch latency vs. wake overhead |
| `PWR_CAN_WINDOW_MS` | 150 | Must catch ≥1 broadcast per unit |
| `PWR_CAN_WINDOW_PERIOD_MS` | 3000 | Duty cycle vs. wake latency. ~1 % duty ≈ 0.12 mA |

## Smaller fixes

- **GPIO4 (CTP_IRQ)** was left as a push-pull output driving LOW after the touch
  reset, while GT911 uses the same pin as its data-ready *output* — a direct
  driver conflict. Now released as an input.
- **CH422G IO0/IO5** are DI0/DI1 *inputs* with 4.7K pull-ups, but the expander's
  output-enable is a single global bit, so they were driven LOW: ~0.7 mA each,
  continuously. Now held HIGH. Guarded by `BOARD_DI_INPUTS_UNUSED` — **set it to
  0 if the DI0/DI1 terminals are ever wired**, or an active opto will fight the
  expander output (limited to ~33 mA by R19/R23).
- Touch-poll logging ran every 1.25 s through standby (two separate log sites).
  Reduced to every 240 polls.

## Traps — do not repeat these

### Never drive IO44 (RS485_TXD) high

This was tried and it cost **+5 mA**. U7 (SP3485EN) has DE and /RE tied
together, pulled high by R66 (4.7K) and pulled low by S1 (8050 NPN, emitter to
GND). S1's base hangs on IO44 through **R68 = 100 Ω only**, so driving IO44 high
sinks (3V3 − Vbe)/100 ≈ 26 mA continuously.

Putting the transceiver in receive-only costs more than leaving its driver
enabled into an idle bus. The cheapest state is to leave IO44 alone.

### CAN silent mode is not available on this board

The TJA1051T/3's S pin (8) is tied to SGND through **R96 (0 Ω)** — permanently
Normal mode, with no software control. And the TJA1051 has no low-power standby
at all; Silent mode only disables the transmitter (~5 mA either way).

### Flashing leaves the chip in ROM download mode

`idf.py flash` ends with "Hard resetting via RTS pin", and on the ESP32-S3's
USB-Serial-JTAG the RTS/DTR lines drive both EN and GPIO0 — the reset lands in
download mode instead of booting. **Power-cycle after every flash.**

Confirm with `esptool --before no-reset chip-id`: if ROM answers without a
reset, the app is not running.

Likewise use `idf.py monitor --no-reset`, and never open the port with a raw
serial library — that also strands the chip in download mode.

### Measure with USB disconnected

`CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION=y` makes USB Serial/JTAG hold an
`ESP_PM_NO_LIGHT_SLEEP` lock the entire time a host is attached. Any measurement
taken over USB shows active-mode current regardless of what the firmware does.

## Instrumentation

`CONFIG_PM_LIGHT_SLEEP_CALLBACKS=y` is enabled and the firmware accumulates real
light-sleep time via `esp_pm_light_sleep_register_cbs()`. The standby heartbeat
reports it:

```
[HB] alive t=... pwr=2 ble=0 twai=0 | 10s: loops=40 canwake=0 win=1 \
     | lightsleep: n=1240 total=95s = 91% of uptime
```

- `twai=0` — driver uninstalled, PM lock released.
- `loops` — deep-idle iterations per 10 s (expect ~40 at 250 ms).
- `canwake` — how many followed a CANRX wake (0 with the wake source disabled).
- `win` — how many actually brought the bus up.
- `lightsleep` — **cumulative since boot, deliberately not reset**, so the
  numbers survive until USB is plugged back in to read them.

`esp_pm_dump_locks(stdout)` is also printed on entry to standby. Anything
permanent in that list other than `usb_serial_jtag` means sleep is still blocked.

## What is left

Roughly 1–1.5 mA is reachable in software, both with a cost:

- CAN window 100 ms / 5 s instead of 150 ms / 3 s: ~2 % duty instead of ~5 %,
  ≈ 0.35 mA, at up to 5 s wake latency.
- Touch polling at 500 ms instead of 250 ms may let the GT911 drop into its own
  green mode (~1.6 mA instead of ~3.5 mA at 3V3), ≈ 0.5 mA, at double the touch
  latency.

The rest is hardware, and **5 mA is not reachable without addressing the CAN
transceiver**:

| Item | Approx. @12 V | Way out |
|---|---|---|
| TJA1051 CAN transceiver | ~2 mA | Swap for pin-compatible TJA1042 / TJA1057 (real STB, ~10 µA), or make its 5 V switchable |
| SP3485 RS485 transceiver | ~0.5–0.8 mA | Depopulate, or switch its supply |
| DISP pull-up R2 (4.7K to 5 V) | ~0.4 mA | Unavoidable while the 5 V rail is up |

## Unrelated finding worth acting on

`CONFIG_SPIRAM_SPEED_120M` with octal PSRAM is marked **experimental** in IDF:
if the chip powers on at one temperature and then drifts ~20 °C, PSRAM accesses
can crash randomly (`esp_psram/esp32s3/Kconfig.spiram`). For a product in a
vehicle, 80 MHz is the safer setting.
