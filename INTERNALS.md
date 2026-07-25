# hw-lilygo-t3s3-sx1262 — INTERNALS

Maintainer reference for the LilyGo T3-S3 (SX1262) board HAL. For what the
straddle is and how to build with it, read [README.md](README.md) first.

## 1. Bring-up: the CS park, and why it runs in the `start:` band

The only firmware this straddle owns is `T3s3Board::onStart`
([esp-idf/src/t3s3.cpp](esp-idf/src/t3s3.cpp)). It configures the SX1262's
chip-select (`CONFIG_LORA0_CS_PIN`, GPIO 7) as an output and drives it HIGH —
deselected — so the radio does not drive the MISO line before `loraInit()` (in
[iface-lora](../iface-lora)) claims the SPI pins.

It runs in the `start:` band, **before** `spangapInit()`, because the radio must
be parked before any SPI bus setup touches those pins. The dispatcher that calls
it is generated into the `reticulous/reticulous` buildable from this straddle's
`services:` declaration — there is no hand-written `app_main` here.

The CS park is guarded by `#if defined(CONFIG_LORA0_CS_PIN)`, so the source still
compiles if the board is ever staged without `iface-lora` (the symbol is defined
only when that straddle is in the build).

Unlike the Heltec V4 there is **no** Vext-style peripheral power rail to bring
up. The microSD card is **not** parked here: it sits on its own SPI bus and its
CS is claimed by `fs_mount_sd()` (called from `spangapInit()`, after this hook),
so nothing contends for it beforehand.

## 2. Two independent SPI buses

The T3-S3 wires the radio and the SD slot to **different** pin sets, so they take
**different** SPI hosts — this is not the T-Deck's shared-bus arrangement:

- **SX1262** — `CONFIG_LORA_SPI_HOST=2` (SPI2/FSPI), SCK 5 / MOSI 6 / MISO 3,
  CS 7. Owned by iface-lora.
- **microSD** — `CONFIG_SPANGAP_SDCARD_SPI_HOST=3` (SPI3/HSPI), SCK 14 / MOSI 11
  / MISO 2, CS 13. Owned by spangap-core's `fs.cpp`
  (`SPANGAP_SDCARD_SPI_HOST` must be 2 or 3; the value is the peripheral name).

Because the two are on separate hosts there is no `spi_helper` arbitration
between them — each `spi_bus_initialize` targets a distinct controller and pin
set. Keep them on distinct hosts: routing both to host 2 would demand one pin
set, which the two buses do not share.

## 3. The radio-variant split

This straddle is the **SX1262** profile. The SX127x (SX1272/76/78) and SX128x
(SX1280) T3-S3 boards are genuinely different hardware from the firmware's side,
which is why they get their own straddle rather than a `when:`-gated block here:

- **SX126x (this board)** — command-based SPI with a **BUSY** handshake line and
  a **DIO1** IRQ; the chip drives **DIO2** as the antenna RF switch and takes a
  **DIO3 TCXO** control voltage. Config: `CONFIG_LORA0_RADIO_SX1262`,
  `CONFIG_LORA0_BUSY_PIN=34`, `CONFIG_LORA0_TCXO_MV=1800`,
  `CONFIG_LORA0_DIO2_RF_SWITCH=y`.
- **SX127x (SX1272/76/78)** — register-based, **no BUSY line** (`BUSY = -1`), IRQ
  on **DIO0** (wired to `LORA0_DIO1_PIN`, GPIO 9 on this board per the Meshtastic
  variant), **no** chip TCXO and **no** DIO2 switch; the antenna path uses
  external RX/TX-enable GPIOs (10/21, populated behind option resistors). SX1272
  and SX1276 differ only in frequency coverage/bandwidth — both map to
  `CONFIG_LORA0_RADIO_SX1276`/`SX1278` with the flags above.
- **SX128x (SX1280)** — 2.4 GHz; different DIO map and freq/bandwidth range.

A future `hw-lilygo-t3s3-sx127x` (or a radio-variant knob) would carry that
alternate pin/flag set; do not try to fold it into this file's `kconfig:` — the
BUSY/DIO/TCXO differences are not a matter of a few pin numbers.

## 4. Everything else is Kconfig VALUES, not sources

A board straddle is **non-buildable**: a `sdkconfig.defaults` here would be
ignored under `--with`. So the hardware profile lives entirely in
`straddle.yaml`'s `kconfig:` block, collected into the buildable's staged
fragments. Three groups:

- **Memory** — `CONFIG_ESPTOOLPY_FLASHSIZE_4MB`, `CONFIG_SPIRAM_MODE_QUAD`,
  `CONFIG_SPANGAP_MAX_FIRMWARE_KB=3584`. The firmware floor keeps a `/state`
  partition alive on the 4 MB chip (~512 KB; bulk data belongs on the SD card).
- **LoRa** — the `CONFIG_LORA*` pins and radio flags, owned by
  [iface-lora](../iface-lora).
- **SD card** — the `CONFIG_SPANGAP_SDCARD*` pins and bus selection, owned by
  [spangap-core](../spangap-core)'s `fs.cpp`.

This board defines no new symbols; it only supplies values.

## 5. Sourcing & pitfalls

The pin map and memory profile were reconstructed from LilyGo's T3-S3 wiki and
the community `tlora_t3s3_v1` Meshtastic variant, **not** from a board in hand.
Nobody has flashed reticulous on a T3-S3 through this straddle yet. Known things
to confirm on real hardware:

- **Radio variant** — the profile assumes an **SX1262**. An SX1276/SX1280 unit
  will not initialise with these flags (see §3).
- **4 MB flash is tight.** The firmware floor sits at 3.5 MB, so `/state` gets
  only ~512 KB and there is no room for an A/B OTA pair. If your module is
  larger, raise the flash-size and firmware-floor Kconfig together.
- **Internal-DRAM headroom.** Quad PSRAM plus WiFi/lwIP plus the SD bounce buffer
  all draw on the internal pool; watch for `ESP_ERR_NO_MEM` under a busy
  WiFi + SD workload, as on other quad-PSRAM boards.
