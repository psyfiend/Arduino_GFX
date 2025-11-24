#include "pin_config.h"
#include <Wire.h>
#include "Arduino_GFX_Library.h"

#include "ESP_I2S.h"

#include "esp_check.h"
#include "es8311.h"
#include "canon.h"

I2SClass i2s;
#define EXAMPLE_SAMPLE_RATE 16000
#define EXAMPLE_VOICE_VOLUME 90
#define EXAMPLE_MIC_GAIN (es8311_mic_gain_t)(3)

Arduino_XCA9554SWSPI *expander = new Arduino_XCA9554SWSPI(
  7,
  0,
  2,
  1,
  &Wire,
  0x20);

esp_err_t es8311_codec_init(void) {
  es8311_handle_t es_handle = es8311_create(0, ES8311_ADDRRES_0);
  ESP_RETURN_ON_FALSE(es_handle, ESP_FAIL, "ES8311", "create failed");

  const es8311_clock_config_t es_clk = {
    .mclk_inverted = false,
    .sclk_inverted = false,
    .mclk_from_mclk_pin = true,
    .mclk_frequency = EXAMPLE_SAMPLE_RATE * 256,
    .sample_frequency = EXAMPLE_SAMPLE_RATE
  };

  ESP_ERROR_CHECK(es8311_init(es_handle, &es_clk, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16));
  ESP_ERROR_CHECK(es8311_sample_frequency_config(es_handle, es_clk.mclk_frequency, es_clk.sample_frequency));
  ESP_ERROR_CHECK(es8311_microphone_config(es_handle, false));
  ESP_ERROR_CHECK(es8311_voice_volume_set(es_handle, EXAMPLE_VOICE_VOLUME, NULL));
  ESP_ERROR_CHECK(es8311_microphone_gain_set(es_handle, EXAMPLE_MIC_GAIN));
  return ESP_OK;
}

void audio_task(void *param) {
  i2s.setPins(PIN_ES7210_BCLK, PIN_ES7210_LRCK, PIN_ES8311_DOUT, PIN_ES7210_DIN, PIN_ES7210_MCLK);
  if (!i2s.begin(I2S_MODE_STD, EXAMPLE_SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO, I2S_STD_SLOT_BOTH)) {
    Serial.println("I2S init failed!");
    vTaskDelete(NULL);
  }

  if (es8311_codec_init() != ESP_OK) {
    Serial.println("ES8311 init failed!");
    vTaskDelete(NULL);
  }

  while (1) {
    i2s.write((uint8_t *)canon_pcm, canon_pcm_len);
    vTaskDelay(1);
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(47, 48);

  expander->pinMode(3, OUTPUT);
  expander->digitalWrite(3, HIGH);
  delay(200);

  xTaskCreatePinnedToCore(audio_task, "audio_task", 4096, NULL, 1, NULL, 1);

  Serial.println("Setup complete.");
}

void loop() {
}
