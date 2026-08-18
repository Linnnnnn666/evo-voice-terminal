#ifndef _EVO_VOICE_V1_BOARD_CONFIG_H_
#define _EVO_VOICE_V1_BOARD_CONFIG_H_

#include <driver/gpio.h>
#include "pin_config.h"

// WiFi 凭据（请替换为你自己的）
#define EVO_WIFI_SSID     "YOUR_WIFI_SSID"
#define EVO_WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
// 服务器 WS 地址（请替换为你自己的 xiaozhi-server 地址）
#define EVO_SERVER_URL    "ws://YOUR_SERVER_HOST/xiaozhi/v1/"

#define AUDIO_INPUT_REFERENCE false
#define AUDIO_INPUT_SAMPLE_RATE 16000
#define AUDIO_OUTPUT_SAMPLE_RATE 16000

// INMP441 麦克风（I2S 输入）
#define AUDIO_MIC_I2S_GPIO_BCLK static_cast<gpio_num_t>(INMP441_BCLK)
#define AUDIO_MIC_I2S_GPIO_WS   static_cast<gpio_num_t>(INMP441_WS)
#define AUDIO_MIC_I2S_GPIO_DATA static_cast<gpio_num_t>(INMP441_DATA)

// MAX98357A 功放（I2S 输出）
#define AUDIO_SPKR_I2S_GPIO_BCLK  static_cast<gpio_num_t>(MAX98357A_BCLK)
#define AUDIO_SPKR_I2S_GPIO_LRCLK static_cast<gpio_num_t>(MAX98357A_LRCLK)
#define AUDIO_SPKR_I2S_GPIO_DATA  static_cast<gpio_num_t>(MAX98357A_DATA)
#define AUDIO_SPKR_ENABLE         static_cast<gpio_num_t>(MAX98357A_SD_MODE)

#define BUILTIN_LED_GPIO        static_cast<gpio_num_t>(EVO_LED_GPIO)
#define BOOT_BUTTON_GPIO        static_cast<gpio_num_t>(EVO_BOOT_GPIO)
#define VOLUME_UP_BUTTON_GPIO   GPIO_NUM_NC
#define VOLUME_DOWN_BUTTON_GPIO GPIO_NUM_NC

#define DISPLAY_BACKLIGHT_PIN GPIO_NUM_NC
#define DISPLAY_MOSI_PIN      GPIO_NUM_NC
#define DISPLAY_SCLK_PIN      GPIO_NUM_NC
#define DISPLAY_DC_PIN        GPIO_NUM_NC
#define DISPLAY_RST_PIN       GPIO_NUM_NC
#define DISPLAY_CS_PIN        GPIO_NUM_NC

#define HAS_SCREEN 0

#endif /* _EVO_VOICE_V1_BOARD_CONFIG_H_ */
