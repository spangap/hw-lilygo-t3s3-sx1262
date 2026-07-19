/**
 * t3s3.cpp — LilyGo T3-S3 (SX1262) board support, end to end.
 *
 * Single owner of all board hardware bring-up. See t3s3.h for the API contract
 * and the board reference: https://wiki.lilygo.cc/products/t3-series/t3-s3/.
 * Layout:
 *
 *   1. LoRa CS park.
 *      Always compiled. Driven from T3s3Board::onStart() before spangapInit().
 *
 * The SX1262 lives on its own SPI bus (SPI2), separate from the flash bus and
 * from the microSD card's bus (SPI3) — so, unlike the T-Deck, there is no
 * shared-bus SD probe to race, and unlike the Heltec V4 there is no Vext rail
 * to bring up. We still park the radio's CS HIGH before loraInit() owns it, so
 * the deselected radio doesn't drive MISO before its driver claims the pin. The
 * SD card is on its own bus and its CS is claimed by fs_mount_sd() (called from
 * spangapInit()), so it needs no pre-park here.
 */
#include "t3s3.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* =========================================================================
 * 1. LoRa CS park
 * ========================================================================= */

static void t3s3PowerInit(void)
{
    /* Park the SX1262's CS HIGH (deselected) so it doesn't drive MISO before
     * loraInit() claims the pin. The LoRa radio CS pin comes from iface-lora's
     * Kconfig (CONFIG_LORA0_CS_PIN); defined only when iface-lora is staged. */
#if defined(CONFIG_LORA0_CS_PIN)
    gpio_config_t cs = {};
    cs.pin_bit_mask = 1ULL << CONFIG_LORA0_CS_PIN;
    cs.mode         = GPIO_MODE_OUTPUT;
    cs.pull_up_en   = GPIO_PULLUP_DISABLE;
    cs.pull_down_en = GPIO_PULLDOWN_DISABLE;
    cs.intr_type    = GPIO_INTR_DISABLE;
    gpio_config(&cs);
    gpio_set_level((gpio_num_t)CONFIG_LORA0_CS_PIN, 1);
#endif
}

/* =========================================================================
 * Public API — the always-on board bring-up (see t3s3.h).
 * ========================================================================= */

void T3s3Board::onStart() {
    t3s3PowerInit();   /* LoRa CS park */
}
