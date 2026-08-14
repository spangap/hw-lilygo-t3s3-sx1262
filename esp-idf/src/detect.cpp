/**
 * detect.cpp — is the hardware under this firmware a LilyGo T3-S3 with an
 * SX1262?
 *
 * The board's one self-assertion. The T3-S3 is a LINE, not a board: the same
 * PCB ships with an SX1262, an SX1276, an SX1280 or an LR1121, and each is its
 * own straddle because the radio driver differs. So this asks the radio which
 * part it is and answers only for the SX1262 — an LR1121 T3-S3 running this
 * image is exactly the mismatch the check exists to catch.
 *
 * See hw-lilygo-tdeck/esp-idf/src/detect.cpp for the contract both callers hold
 * it to, and detect_probe.h for why flashmon's detector carries a hand-kept copy
 * (as `detect_hw_lilygo_t3s3_sx1262`).
 *
 * Passive throughout — no rail to drive, nothing but bus reads.
 */
#include "detect_probe.h"
#include "t3s3.h"

#define DETECT_OLED_SDA   18
#define DETECT_OLED_SCL   17

/* LoRa header (straddle.yaml's CONFIG_LORA0_*, written out: those symbols only
 * exist when iface-lora is staged, and this must probe without it). */
#define DETECT_LORA_SCK    5
#define DETECT_LORA_MOSI   6
#define DETECT_LORA_MISO   3
#define DETECT_LORA_CS     7
#define DETECT_LORA_RST    8
#define DETECT_LORA_BUSY  34

extern "C" const char* detect_hw(void)
{
    if (!detect_flash_mb(4)) return NULL;

    /* Anchor: the OLED, at whichever of the two strapped addresses. */
    if (!detect_ack2(DETECT_OLED_SDA, DETECT_OLED_SCL, 0x3C, 0x3D)) {
        detect_dbg("no OLED on 18/17 — not a T3-S3");
        return NULL;
    }
    /* Which T3-S3: the radio names the straddle here, so nothing but an SX1262
     * is this board. */
    if (!detect_radio_is(DETECT_LORA_SCK, DETECT_LORA_MOSI, DETECT_LORA_MISO,
                         DETECT_LORA_CS, DETECT_LORA_RST, DETECT_LORA_BUSY, "sx1262")) {
        detect_dbg("T3-S3 pins, but the radio is not an SX1262");
        return NULL;
    }

    detect_found("hw_lilygo_t3s3_sx1262");
    return "hw-lilygo-t3s3-sx1262";
}
