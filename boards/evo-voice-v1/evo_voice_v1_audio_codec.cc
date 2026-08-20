#include "evo_voice_v1_audio_codec.h"

#include <esp_log.h>
#include <driver/i2s_std.h>

#include "config.h"

static const char TAG[] = "EvoVoiceV1AudioCodec";

// 输入增益（INMP441 输出幅度实测仅 ~500/32767，放大 16 倍提升唤醒灵敏度）
#define EVO_MIC_GAIN_SHIFT 4  // x16

EvoVoiceV1AudioCodec::EvoVoiceV1AudioCodec(int input_sample_rate, int output_sample_rate,
    gpio_num_t mic_bclk, gpio_num_t mic_ws, gpio_num_t mic_data,
    gpio_num_t spkr_bclk, gpio_num_t spkr_lrclk, gpio_num_t spkr_data,
    bool input_reference) {
    duplex_ = true;
    input_reference_ = input_reference;
    input_channels_ = input_reference_ ? 2 : 1;
    input_sample_rate_ = input_sample_rate;
    output_sample_rate_ = output_sample_rate;

    CreateVoiceHardware(mic_bclk, mic_ws, mic_data, spkr_bclk, spkr_lrclk, spkr_data);

    // MAX98357A SD 引脚：拉高使能功放
    gpio_config_t config;
    config.pin_bit_mask = BIT64(AUDIO_SPKR_ENABLE);
    config.mode = GPIO_MODE_OUTPUT;
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_ENABLE;
    config.intr_type = GPIO_INTR_DISABLE;
#if SOC_GPIO_SUPPORT_PIN_HYS_FILTER
    config.hys_ctrl_mode = GPIO_HYS_SOFT_ENABLE;
#endif
    gpio_config(&config);
    gpio_set_level(AUDIO_SPKR_ENABLE, 0);
    ESP_LOGI(TAG, "EvoVoiceV1AudioCodec initialized (INMP441 + MAX98357A)");
}

EvoVoiceV1AudioCodec::~EvoVoiceV1AudioCodec() {
    if (rx_handle_) {
        i2s_del_channel(rx_handle_);
    }
    if (tx_handle_) {
        i2s_del_channel(tx_handle_);
    }
}

void EvoVoiceV1AudioCodec::CreateVoiceHardware(gpio_num_t mic_bclk, gpio_num_t mic_ws, gpio_num_t mic_data,
    gpio_num_t spkr_bclk, gpio_num_t spkr_lrclk, gpio_num_t spkr_data) {
    i2s_chan_config_t mic_chan_config = I2S_CHANNEL_DEFAULT_CONFIG(XIAOZHI_I2S_PORT(0), I2S_ROLE_MASTER);
    mic_chan_config.auto_clear = true;
    i2s_chan_config_t spkr_chan_config = I2S_CHANNEL_DEFAULT_CONFIG(XIAOZHI_I2S_PORT(1), I2S_ROLE_MASTER);
    spkr_chan_config.auto_clear = true;

    ESP_ERROR_CHECK(i2s_new_channel(&mic_chan_config, NULL, &rx_handle_));
    ESP_ERROR_CHECK(i2s_new_channel(&spkr_chan_config, &tx_handle_, NULL));

    i2s_std_config_t mic_config = {
        .clk_cfg = {
            .sample_rate_hz = (uint32_t)input_sample_rate_,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .mclk_multiple = I2S_MCLK_MULTIPLE_256,
#ifdef I2S_HW_VERSION_2
            .ext_clk_freq_hz = 0,
#endif
        },
        // INMP441 用 16-bit 槽位（BCLK=32*fs=512kHz）：实测 32-bit 槽位（1.024MHz）
        // 下该模块输出异常（8192 步长怪数据），16-bit 槽位输出正常
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = mic_bclk,
            .ws = mic_ws,
            .dout = I2S_GPIO_UNUSED,
            .din = mic_data,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            }
        }
    };

    i2s_std_config_t spkr_config = {
        .clk_cfg = {
            .sample_rate_hz = (uint32_t)output_sample_rate_,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .mclk_multiple = I2S_MCLK_MULTIPLE_256,
#ifdef I2S_HW_VERSION_2
            .ext_clk_freq_hz = 0,
#endif
        },
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = spkr_bclk,
            .ws = spkr_lrclk,
            .dout = spkr_data,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false
            }
        }
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle_, &mic_config));
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle_, &spkr_config));
    // 关键：显式启用通道，否则 i2s_channel_read/write 直接返回
    // ESP_ERR_INVALID_STATE，I2S 外设不启动，BCLK/WS 无时钟输出
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle_));
    ESP_ERROR_CHECK(i2s_channel_enable(tx_handle_));

    ESP_LOGI(TAG, "Voice hardware created (I2S duplex)");
}

void EvoVoiceV1AudioCodec::SetOutputVolume(int volume) {
    volume_ = volume;
    AudioCodec::SetOutputVolume(volume);
}

void EvoVoiceV1AudioCodec::EnableInput(bool enable) {
    AudioCodec::EnableInput(enable);
}

void EvoVoiceV1AudioCodec::EnableOutput(bool enable) {
    if (enable) {
        gpio_set_level(AUDIO_SPKR_ENABLE, 1);
    } else {
        gpio_set_level(AUDIO_SPKR_ENABLE, 0);
    }
    AudioCodec::EnableOutput(enable);
}

int EvoVoiceV1AudioCodec::Read(int16_t *dest, int samples) {
    int64_t t0 = esp_timer_get_time();
    if (input_enabled_) {
        // 16-bit 立体声槽位：DMA 数据交错为 [L, R, L, R, ...]
        // INMP441 24-bit 数据跨 16-bit 槽：L=数据高 16 位（有效），R=LSB 残留（丢弃）
        size_t total = samples;  // 每个 16-bit 输出样本需要 1 帧（2 个 16-bit 字）
        int16_t *tmp = (int16_t *)malloc(total * 2 * sizeof(int16_t));
        if (!tmp) {
            return samples;
        }
        size_t bytes_read = 0;
        esp_err_t err = i2s_channel_read(rx_handle_, tmp, total * 2 * sizeof(int16_t),
                                         &bytes_read, portMAX_DELAY);
        if (err == ESP_OK) {
            size_t n = bytes_read / sizeof(int16_t);
            // 抽取 L 声道 + 输入增益（×16，带限幅）
            for (size_t i = 0; i < n / 2 && i < (size_t)samples; i++) {
                int32_t v = (int32_t)tmp[i * 2] << EVO_MIC_GAIN_SHIFT;
                if (v > 32767) v = 32767;
                if (v < -32768) v = -32768;
                dest[i] = (int16_t)v;
            }
        } else {
            ESP_LOGE(TAG, "i2s read error: 0x%x", err);
        }
        free(tmp);
    }
    int64_t dt = esp_timer_get_time() - t0;
    if (dt > 50000) {
        ESP_LOGI(TAG, "READ SLOW: %lld ms", (long long)(dt / 1000));
    }
    return samples;
}

int EvoVoiceV1AudioCodec::Write(const int16_t *data, int samples) {
    int64_t t0 = esp_timer_get_time();
    if (output_enabled_) {
        size_t bytes_written = 0;
        // 静态缓冲避免每次 malloc（播放路径高频调用）
        // I2S 输出为 16-bit STEREO 槽：mono 样本必须复制为 L=R，
        // 否则 960 个 mono 样本（60ms）会被当 480 帧 stereo（30ms）播放 → 2 倍速怪声
        static int16_t s_output_buf[2048];
        if (samples > 1024) {
            ESP_LOGE(TAG, "write samples too large: %d", samples);
            return samples;
        }
        int32_t vol = volume_;
        if (vol > 100) vol = 100;
        for (size_t i = 0; i < samples; i++) {
            int16_t v = (int16_t)(((int32_t)data[i] * vol) / 100);
            s_output_buf[i * 2] = v;
            s_output_buf[i * 2 + 1] = v;
        }
        esp_err_t err = i2s_channel_write(tx_handle_, s_output_buf, samples * 2 * sizeof(int16_t),
                                          &bytes_written, portMAX_DELAY);
        int64_t dt = esp_timer_get_time() - t0;
        // 全部 %d 打印（避免 Xtensa %lld 参数错位）；每 10 次打一次
        static int wdbg = 0;
        if (++wdbg % 10 == 0 || dt > 50000) {
            ESP_LOGI(TAG, "W n=%d err=%d bw=%u dt=%u d0=%d",
                     samples, err, (unsigned)bytes_written, (unsigned)(dt / 1000), data[0]);
        }
    }
    return samples;
}
