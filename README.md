# hw-lilygo-t3s3-sx1262 — LilyGo T3-S3 (SX1262) board HAL

**hw-lilygo-t3s3-sx1262** is the board-support straddle for the **LilyGo T3-S3**
(LoRa32) — an ESP32-S3 LoRa node built on the **ESP32-S3-WROOM-1(U)**
(ESP32-S3FH4R2: 4 MB flash, 2 MB **quad** PSRAM) carrying one Semtech **SX1262**
on its own SPI bus, plus a microSD slot on a second SPI bus. It makes the board
usable by an application: it owns the LoRa CS park and publishes the board's pin
map and hardware tuning as Kconfig. Board reference:
<https://wiki.lilygo.cc/products/t3-series/t3-s3/>.

It is a **non-buildable** component — it decides nothing about what the device
*does*. A buildable assembler (`reticulous/reticulous`) adds it and inherits the
board: `spangap build reticulous/reticulous --with spangap/hw-lilygo-t3s3-sx1262`. The
mesh stack, the IP/web platform, `app_main`, the partition layout, the update
story and the browser SPA all come from the buildable and its other straddles —
not from here.

This straddle targets the **SX1262** (sub-GHz SX126x) radio variant. It brings
up **LoRa + microSD + the 0.96" SSD1306 OLED**, the latter as a paged status
display via [tinylcd](../tinylcd) (staged by this board through
`additional_installs`, pins fed through a gated `kconfig:` group); the BOOT
button (GPIO 0) advances the page. The colour-TFT UI (`spangap-lcd`) stays out
— the OLED is tinylcd's mono paged UI, not LVGL.

## Which T3-S3 — radio variants

LilyGo ships the T3-S3 with several radios. **This straddle is for the SX1262
only.** The others differ enough to warrant their own board straddle:

| Variant | Family | Band | How it differs (firmware-relevant) |
|---|---|---|---|
| **SX1262** *(this straddle)* | SX126x | 868/915 MHz | BUSY line + DIO1 IRQ, chip-driven 1.8 V TCXO (DIO3), DIO2 as RF switch |
| SX1276 / SX1278 | SX127x | 868/915 · 433 MHz | **No BUSY line** (set `-1`), IRQ on **DIO0**, no chip TCXO, no DIO2 switch; antenna via external RX/TX-enable GPIOs (10/21, resistor-optioned) |
| SX1280 | SX128x | 2.4 GHz | Higher data rate, no sub-GHz; different DIO map and freq/bandwidth range |

SX1272 vs SX1276 (both SX127x) is only a frequency-coverage/bandwidth
difference — from the firmware's view both are register-based SX127x parts
(`CONFIG_LORA0_RADIO_SX1276`/`SX1278`, `BUSY = -1`, IRQ on the DIO0 GPIO, no
`TCXO`/`DIO2_RF_SWITCH`). A T3-S3 with one of those needs a sibling straddle with
that pin/flag set — it is **not** a drop-in for this SX1262 profile.

## ⚠️ Verify before trusting

The pin map below was assembled from LilyGo's T3-S3 wiki and the community
`tlora_t3s3_v1` Meshtastic variant, **not** from a board in hand. Confirm against
your actual unit before an RF or partition run:

- **Radio variant.** Make sure your board is the **SX1262** version. An SX1276/
  SX1280 unit will not initialise with this profile (see the table above).
- **LoRa pin map.** Taken from the `tlora_t3s3_v1` variant. Re-check every
  `CONFIG_LORA*` pin if your unit is a different T3-S3 revision.
- **Flash size.** 4 MB / 2 MB quad PSRAM (ESP32-S3FH4R2). 4 MB is tight — the
  firmware floor leaves only 256 KB for `/state` and no room for an A/B OTA
  pair. If your unit carries a larger module, bump `CONFIG_ESPTOOLPY_FLASHSIZE_*`
  and `CONFIG_SPANGAP_MAX_FIRMWARE_KB` together.

## What it does, and how it fits

The board contributes one hook that the buildable's generated init dispatcher
calls. There is nothing to call by hand: if the straddle is in the build, the
board comes up automatically.

| Hook | Band | Present when | Brings up |
|---|---|---|---|
| `T3s3Board::onStart` | start | always | LoRa CS park HIGH |

`onStart` runs in the `start:` band, **before** `spangapInit()`. It is
bare-hardware bring-up: it parks the SX1262's CS line HIGH so the radio does not
drive MISO before `loraInit()` (in [iface-lora](../iface-lora)) claims the pin.
There is no `init:`-band companion — the OLED UI is [tinylcd](../tinylcd)'s own
service, not a board hook. The microSD card is mounted by `spangapInit()`'s
`fs_mount_sd()` (it sits on its own SPI bus, so its CS needs no pre-park). The
board has no gated peripheral power rail (unlike the Heltec V4's Vext), so there
is nothing else to power up at boot.

The LoRa radio engine, the IP/web platform and the mesh stack are owned by other
straddles ([iface-lora](../iface-lora), [spangap-core](../spangap-core),
[spangap-net](../spangap-net), [rns](../rns)); this board only supplies the
SX1262's pins (below), the SD pins and the CS glue.

## Board identity (`detect_hw`)

`esp-idf/src/detect.cpp` answers one question about this board: it returns
`"hw-lilygo-t3s3-sx1262"` when the hardware under the firmware is this board, and NULL when
it is not. What it asks:

4 MB flash, then the OLED acking on 18/17, and the radio must read as an
**SX1262** — the modem is what names the straddle here, so the same PCB carrying
an LR1121 or an SX1280 answers NULL and gets its own board straddle. Passive
reads only; no rail is driven, which is why this board is probed before the ones
that drive one.

spangap-core calls it before the first `onStart()` — the last moment no bus is
claimed — and **halts the device awake** when the answer disagrees with the board
this image was built for, since every pin map here would then belong to someone
else's hardware. The confirmed answer is published as `sys.hw` and announced on
the console as `build: hw hw-lilygo-t3s3-sx1262`. flashmon's standalone detector carries a
hand-kept copy of the same function, renamed `detect_hw_lilygo_t3s3_sx1262`, to identify a chip
whose firmware is unknown; change one, change the other. See
[spangap-core/docs/init.md](../spangap-core/docs/init.md) and
[flashmon/docs/detect.md](../flashmon/docs/detect.md).

## Hardware & pin map

LilyGo **T3-S3** — ESP32-S3-WROOM-1(U) module (4 MB flash, 2 MB **quad** PSRAM,
selected via `CONFIG_SPIRAM_MODE_QUAD`). Two independent SPI buses: the SX1262
on **SPI host 2** (FSPI) and the microSD on **SPI host 3** (HSPI).

### LoRa SX1262 (owned by iface-lora, pins published here)

| Signal | GPIO | | Signal | GPIO |
|---|---|---|---|---|
| NSS / CS | 7 | | RST | 8 |
| SCK | 5 | | BUSY | 34 |
| MOSI | 6 | | DIO1 | 33 |
| MISO | 3 | | | |

The SX1262 drives **DIO2** as its own RF antenna switch
(`CONFIG_LORA0_DIO2_RF_SWITCH=y`) and **DIO3** supplies the 1.8 V TCXO
(`CONFIG_LORA0_TCXO_MV=1800`). One radio (`CONFIG_LORA_COUNT=1`),
`CONFIG_LORA0_RADIO_SX1262=y`.

### microSD (owned by spangap-core, pins published here)

| Signal | GPIO |
|---|---|
| CS | 13 |
| SCK | 14 |
| MOSI | 11 |
| MISO | 2 |

On its own SPI bus (**host 3**), separate from the radio's — so no bus
arbitration with LoRa. Mounted at boot when `CONFIG_SPANGAP_SDCARD=y`.

### OLED + page button (owned by tinylcd, pins published here)

| Signal | GPIO | Notes |
|---|---|---|
| OLED SDA / SCL | 18 / 17 | 0.96" SSD1306 128×64, address 0x3C, no reset line |
| page button (BOOT) | 0 | click = next status page; 500 ms hold = screen off; press wakes a dark screen |

### Memory / flash (published from `kconfig:`)

A non-buildable straddle has no `sdkconfig.defaults` of its own — it would be
ignored under `--with` — so every value that describes this hardware is
published from `straddle.yaml`'s `kconfig:` block and consumed by the owning
straddle / IDF:

| Key | Value | Why |
|---|---|---|
| `CONFIG_ESPTOOLPY_FLASHSIZE_4MB` | `y` | 4 MB flash (ESP32-S3FH4R2) — **verify** |
| `CONFIG_SPANGAP_MAX_FIRMWARE_KB` | `3840` | state floor at 3.75 MB: the reticulous binary (with the SD/FAT driver) fits the `app` slot alongside the ~704 KB `fixed` partition below it; `/state` keeps the remaining 256 KB, which is enough because bulk data belongs on the microSD card. Without it `app` eats all 4 MB — leaving **no `/state`** |
| `CONFIG_SPIRAM_MODE_QUAD` | `y` | the S3FH4R2 carries 2 MB PSRAM in **quad** mode, not octal |

The platform's usual "octal PSRAM" assumption (the T-Deck's S3R8) does **not**
hold here — watch internal-DRAM headroom for DMA/WiFi/lwIP, as on the Heltec V4.
The SD card's bounce buffer draws on that same internal pool, so a busy WiFi +
SD workload is the first place to look if allocations start failing.

## Storage variables

This board defines no storage keys of its own. Runtime LoRa parameters live at
`s.lora.*` ([iface-lora](../iface-lora)); the SD mount is owned by
[spangap-core](../spangap-core).

## Dependencies

- [spangap-core](../spangap-core) — base runtime (storage, log, CLI, fs incl. the
  SD mount, ITS).
- [iface-lora](../iface-lora) — owns the SX1262 radio engine; this board parks
  its CS and supplies its pins via Kconfig.
- [tinylcd](../tinylcd) — staged by this board (`additional_installs`); owns
  the OLED paged UI and the page button, pins supplied via Kconfig.

## Read next

- [INTERNALS.md](INTERNALS.md) — the CS-park bring-up, the two-bus SPI layout,
  the radio-variant split, and the board pitfalls.
