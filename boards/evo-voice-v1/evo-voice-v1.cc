#include "wifi_board.h"
#include "evo_voice_v1_audio_codec.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "led/single_led.h"

#include <esp_log.h>
#include <nvs.h>

#define TAG "EvoVoiceV1Board"

// EvoAgent Voice Board v1：ESP32-S3 N16R8 + INMP441（麦克风）+ MAX98357A（功放）
// 干净重建版：无 I2C codec、无屏幕，纯语音板
class EvoVoiceV1Board : public WifiBoard {
private:
    EvoVoiceV1AudioCodec audio_codec_;
    Button boot_button_;
    SingleLed status_led_;

    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting) {
                EnterWifiConfigMode();
                return;
            }
            app.ToggleChatState();
        });
    }

    // 预置 WiFi 凭据（首次启动写入 NVS "wifi" namespace；已配置则跳过）
    void PresetWifiCredentials() {
        nvs_handle_t nvs;
        if (nvs_open("wifi", NVS_READWRITE, &nvs) != ESP_OK) {
            return;
        }
        char existing[64] = {0};
        size_t len = sizeof(existing);
        if (nvs_get_str(nvs, "ssid", existing, &len) != ESP_OK) {
            nvs_set_str(nvs, "ssid", EVO_WIFI_SSID);
            nvs_set_str(nvs, "password", EVO_WIFI_PASSWORD);
            nvs_commit(nvs);
            ESP_LOGI(TAG, "WiFi credentials preset");
        }
        nvs_close(nvs);
    }

public:
    EvoVoiceV1Board()
        : audio_codec_(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
                       AUDIO_MIC_I2S_GPIO_BCLK, AUDIO_MIC_I2S_GPIO_WS, AUDIO_MIC_I2S_GPIO_DATA,
                       AUDIO_SPKR_I2S_GPIO_BCLK, AUDIO_SPKR_I2S_GPIO_LRCLK, AUDIO_SPKR_I2S_GPIO_DATA,
                       AUDIO_INPUT_REFERENCE),
          boot_button_(BOOT_BUTTON_GPIO),
          status_led_(BUILTIN_LED_GPIO) {
        InitializeButtons();
        // PresetWifiCredentials(); // 暂时禁用：需在 nvs_flash_init 之后调用（否则启动崩溃）
        ESP_LOGI(TAG, "EvoVoiceV1Board initialized (INMP441 + MAX98357A, no display)");
    }

    virtual AudioCodec* GetAudioCodec() override { return &audio_codec_; }
    virtual Led* GetLed() override { return &status_led_; }
};

DECLARE_BOARD(EvoVoiceV1Board);
