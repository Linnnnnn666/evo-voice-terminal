/*
 * EvoAgent Voice Board v1 (ESP32-S3 N16R8 + INMP441 + MAX98357A)
 * 干净重建语音板：无 I2C codec 芯片，纯 I2S 直连
 *   - 麦克风：INMP441（I2S MEMS）
 *   - 喇叭：MAX98357A（I2S Class-D 功放）
 * 参考上游 lilygo t-circle-s3 的 I2S 实现（MIT）
 */
#ifndef _EVO_VOICE_V1_PIN_CONFIG_H_
#define _EVO_VOICE_V1_PIN_CONFIG_H_

// INMP441 麦克风（I2S 输入）——注意：避开 GPIO33-37（PSRAM 总线占用）与 GPIO26-32（flash）
// 2026-08-25：BCLK/WS 从 GPIO15/16 换成 GPIO12/13——GPIO15/16 实测输出异常频率
// （1.126MHz/120kHz），疑似板载信号占用；12/13 为干净引脚
#define INMP441_BCLK 12
#define INMP441_WS   13
#define INMP441_DATA 21

// MAX98357A 功放（I2S 输出）
#define MAX98357A_BCLK   4
#define MAX98357A_LRCLK  14
#define MAX98357A_DATA   18
#define MAX98357A_SD_MODE 5

// 板载
#define EVO_LED_GPIO 2
#define EVO_BOOT_GPIO 0

#endif /* _EVO_VOICE_V1_PIN_CONFIG_H_ */
