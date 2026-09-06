#include "Arduino_ESP32DSIPanel.h"

#define TAG "Arduino_ESP32DSIPanel"

#if defined(ESP32) && (CONFIG_IDF_TARGET_ESP32P4)
Arduino_ESP32DSIPanel::Arduino_ESP32DSIPanel(
    uint32_t hsync_pulse_width, uint32_t hsync_back_porch, uint32_t hsync_front_porch,
    uint32_t vsync_pulse_width, uint32_t vsync_back_porch, uint32_t vsync_front_porch,
    uint32_t prefer_speed,uint32_t lane_bit_rate /*新增成员变量*/,
    uint8_t phy_clk_src, uint8_t num_fb /* Fleet */)
    : _hsync_pulse_width(hsync_pulse_width), _hsync_back_porch(hsync_back_porch), _hsync_front_porch(hsync_front_porch),
      _vsync_pulse_width(vsync_pulse_width), _vsync_back_porch(vsync_back_porch), _vsync_front_porch(vsync_front_porch),
      _prefer_speed(prefer_speed),
	  _lane_bit_rate(lane_bit_rate)/*新增成员变量*/,
      _phy_clk_src(phy_clk_src) /* Fleet */,
      _num_fb(num_fb) /* Fleet */
{
}

bool Arduino_ESP32DSIPanel::begin(int16_t w, int16_t h, int32_t speed, const lcd_init_cmd_t *init_operations, size_t init_operations_len)
{
  if (speed == GFX_NOT_DEFINED)
  {
    if (_prefer_speed != GFX_NOT_DEFINED)
    {
      speed = _prefer_speed;
    }
    else
    {
      speed = 56000000L;
    }
  }

  esp_ldo_channel_handle_t ldo_mipi_phy = NULL;
  esp_ldo_channel_config_t ldo_mipi_phy_config = {
      .chan_id = EXAMPLE_MIPI_DSI_PHY_PWR_LDO_CHAN,
      .voltage_mv = EXAMPLE_MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV,
  };
  ESP_ERROR_CHECK(esp_ldo_acquire_channel(&ldo_mipi_phy_config, &ldo_mipi_phy));
  ESP_LOGI(TAG, "MIPI DSI PHY Powered on");

  // Fleet: board-selected PHY PLL reference clock. The BSP default of 0 keeps
  // this library's long-standing PLL_F20M choice, so boards that do not set
  // DisplayConfig.PHY_CLK_SRC are bit-for-bit unchanged by this addition.
  mipi_dsi_phy_pllref_clock_source_t phy_src = MIPI_DSI_PHY_PLLREF_CLK_SRC_PLL_F20M;
  switch (_phy_clk_src)
  {
  case 1: phy_src = (mipi_dsi_phy_pllref_clock_source_t)0; break; // let IDF choose
  case 2: phy_src = MIPI_DSI_PHY_PLLREF_CLK_SRC_PLL_F20M; break;
  case 3: phy_src = MIPI_DSI_PHY_PLLREF_CLK_SRC_PLL_F25M; break;
  case 4: phy_src = MIPI_DSI_PHY_PLLREF_CLK_SRC_RC_FAST; break;
  case 5: phy_src = MIPI_DSI_PHY_PLLREF_CLK_SRC_XTAL; break;
  default: break; // 0 / unknown -> library default above
  }
  ESP_LOGI(TAG, "PHY PLL ref clk: BSP selector=%u -> enum %d", (unsigned)_phy_clk_src, (int)phy_src);

  esp_lcd_dsi_bus_config_t bus_config = {
      .bus_id = 0,
      .num_data_lanes = 2,
      .phy_clk_src = phy_src,
      .lane_bit_rate_mbps = _lane_bit_rate, /*新增成员变量*/
  };
  esp_lcd_dsi_bus_handle_t mipi_dsi_bus = NULL;

  ESP_ERROR_CHECK(esp_lcd_new_dsi_bus(&bus_config, &mipi_dsi_bus));

  ESP_LOGI(TAG, "Install MIPI DSI LCD control panel");
  esp_lcd_dbi_io_config_t dbi_config = {
      .virtual_channel = 0,
      .lcd_cmd_bits = 8,
      .lcd_param_bits = 8,
  };
  esp_lcd_panel_io_handle_t io_handle = NULL;
  ESP_LOGI(TAG, "STEP -> esp_lcd_new_panel_io_dbi");
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_dbi(mipi_dsi_bus, &dbi_config, &io_handle));
  ESP_LOGI(TAG, "STEP dbi-io done");

  esp_lcd_dpi_panel_config_t dpi_config = {
      .virtual_channel = 0,
      .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
      .dpi_clock_freq_mhz = speed / 1000000,
      .pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB565,
      .num_fbs = (_num_fb ? _num_fb : 1), /* Fleet */
      .video_timing = {
          .h_size = w,
          .v_size = h,
          .hsync_pulse_width = _hsync_pulse_width,
          .hsync_back_porch = _hsync_back_porch,
          .hsync_front_porch = _hsync_front_porch,
          .vsync_pulse_width = _vsync_pulse_width,
          .vsync_back_porch = _vsync_back_porch,
          .vsync_front_porch = _vsync_front_porch,
      },
      .flags = {
          .use_dma2d = true,
      },
  };

  const esp_lcd_panel_dev_config_t panel_config = {
      .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
      .bits_per_pixel = 16,
  };

  // Create MIPI DPI panel
  ESP_LOGI(TAG, "STEP -> esp_lcd_new_panel_dpi (fbs=%d[BSP=%u], %dx%d @ %luMHz)", dpi_config.num_fbs, (unsigned)_num_fb, (int)w, (int)h, (unsigned long)(speed/1000000));
  ESP_ERROR_CHECK(esp_lcd_new_panel_dpi(mipi_dsi_bus, &dpi_config, &_panel_handle));
  ESP_LOGI(TAG, "STEP dpi-panel done");

  ESP_LOGI(TAG, "STEP -> init cmd loop (%u cmds)", (unsigned)init_operations_len);
  for (int i = 0; i < init_operations_len; i++)
  {
    // Send command
    ESP_LOGI(TAG, "  cmd[%d] = 0x%02X", i, (unsigned)init_operations[i].cmd);
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, init_operations[i].cmd, init_operations[i].data, init_operations[i].data_bytes));
    vTaskDelay(pdMS_TO_TICKS(init_operations[i].delay_ms));
  }
  ESP_LOGD(TAG, "send init commands success");

  ESP_ERROR_CHECK(esp_lcd_panel_init(_panel_handle));

  return true;
}

uint16_t *Arduino_ESP32DSIPanel::getFrameBuffer()
{
  void *frame_buffer = nullptr;
  ESP_ERROR_CHECK(esp_lcd_dpi_panel_get_frame_buffer(_panel_handle, 1, &frame_buffer));

  return ((uint16_t *)frame_buffer);
}

#endif // #if defined(ESP32) && (CONFIG_IDF_TARGET_ESP32P4)
