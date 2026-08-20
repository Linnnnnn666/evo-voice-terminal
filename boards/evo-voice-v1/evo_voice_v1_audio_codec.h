#ifndef _EVO_VOICE_V1_AUDIO_CODEC_H
#define _EVO_VOICE_V1_AUDIO_CODEC_H

#include "audio_codec.h"

#include <driver/i2s_std.h>

// EvoAgent Voice Board v1：INMP441（麦克风 I2S 输入）+ MAX98357A（功放 I2S 输出）
// 无 I2C codec 芯片；参考上游 lilygo t-circle-s3 的 I2S 直连实现（MIT）
class EvoVoiceV1AudioCodec : public AudioCodec {
private:
    i2s_chan_handle_t rx_handle_ = nullptr;
    i2s_chan_handle_t tx_handle_ = nullptr;
    uint32_t volume_ = 70;

    void CreateVoiceHardware(gpio_num_t mic_bclk, gpio_num_t mic_ws, gpio_num_t mic_data,
                             gpio_num_t spkr_bclk, gpio_num_t spkr_lrclk, gpio_num_t spkr_data);

    virtual int Read(int16_t *dest, int samples) override;
    virtual int Write(const int16_t *data, int samples) override;

public:
    EvoVoiceV1AudioCodec(int input_sample_rate, int output_sample_rate,
        gpio_num_t mic_bclk, gpio_num_t mic_ws, gpio_num_t mic_data,
        gpio_num_t spkr_bclk, gpio_num_t spkr_lrclk, gpio_num_t spkr_data,
        bool input_reference);
    virtual ~EvoVoiceV1AudioCodec();

    virtual void SetOutputVolume(int volume) override;
    virtual void EnableInput(bool enable) override;
    virtual void EnableOutput(bool enable) override;
};

#endif /* _EVO_VOICE_V1_AUDIO_CODEC_H */
