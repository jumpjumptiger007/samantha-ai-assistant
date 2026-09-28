#include "wifi_board.h"
#include "display/lcd_display.h"
#include "codecs/es8311_audio_codec.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "assets/lang_config.h"
#include "cw2017_battery_monitor.h"
#include "display/lvgl_display/lvgl_theme.h"

#include <esp_log.h>
#include <esp_lcd_panel_vendor.h>
#include <button_adc.h>
#include <esp_adc/adc_oneshot.h>
#include <driver/i2c_master.h>
#include <driver/spi_common.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cstring>

#define TAG "AiPassport"

// Physical keys share one ADC pin through a resistor ladder (see config.h).
enum {
    kAdcButtonUp = 0,
    kAdcButtonDown,
    kAdcButtonOk,
    kAdcButtonNum,
};

class SamanthaPassportDisplay final : public SpiLcdDisplay {
public:
    SamanthaPassportDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel,
                            int width, int height, int offset_x, int offset_y, bool mirror_x,
                            bool mirror_y, bool swap_xy)
        : SpiLcdDisplay(panel_io, panel, width, height, offset_x, offset_y, mirror_x, mirror_y,
                        swap_xy) {
        ApplySamanthaPalette(GetTheme());
    }

    void SetupUI() override {
        SpiLcdDisplay::SetupUI();

        DisplayLockGuard lock(this);
        const lv_color_t coral = lv_color_hex(0xD1684E);
        const lv_color_t white = lv_color_hex(0xFFFFFF);
        lv_obj_t* screen = lv_screen_active();
        if (screen != nullptr) {
            lv_obj_set_style_bg_color(screen, coral, 0);
            lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
        }
        if (container_ != nullptr) {
            lv_obj_set_style_bg_color(container_, coral, 0);
            lv_obj_set_style_bg_opa(container_, LV_OPA_COVER, 0);
        }
        if (content_ != nullptr) {
            lv_obj_set_style_bg_color(content_, coral, 0);
            lv_obj_set_style_bg_opa(content_, LV_OPA_TRANSP, 0);
        }
        if (top_bar_ != nullptr) {
            lv_obj_set_style_bg_opa(top_bar_, LV_OPA_TRANSP, 0);
        }
        if (status_bar_ != nullptr) {
            lv_obj_set_style_bg_opa(status_bar_, LV_OPA_TRANSP, 0);
        }
        if (status_label_ != nullptr) {
            lv_obj_set_style_text_color(status_label_, white, 0);
        }
        if (notification_label_ != nullptr) {
            lv_obj_set_style_text_color(notification_label_, white, 0);
        }
        if (emoji_box_ != nullptr) {
            lv_obj_set_size(emoji_box_, 128, 128);
            lv_obj_align(emoji_box_, LV_ALIGN_CENTER, 0, 0);
        }
        if (bottom_bar_ != nullptr) {
            lv_obj_set_style_bg_color(bottom_bar_, coral, 0);
            lv_obj_set_style_bg_opa(bottom_bar_, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(bottom_bar_, 0, 0);
            lv_obj_set_style_pad_all(bottom_bar_, 0, 0);
        }
        if (chat_message_label_ != nullptr) {
            lv_obj_set_style_text_color(chat_message_label_, white, 0);
            lv_obj_set_style_text_align(chat_message_label_, LV_TEXT_ALIGN_CENTER, 0);
        }
    }

    void SetStatus(const char* status) override {
        const char* emotion = nullptr;
        if (status != nullptr && std::strcmp(status, Lang::Strings::STANDBY) == 0) {
            emotion = "idle";
        } else if (status != nullptr && std::strcmp(status, Lang::Strings::LISTENING) == 0) {
            emotion = "listening";
        } else if (status != nullptr && std::strcmp(status, Lang::Strings::SPEAKING) == 0) {
            emotion = "speaking";
        }

        if (emotion == nullptr) {
            SpiLcdDisplay::SetStatus(status);
            return;
        }

        // Keep operational/provisioning statuses visible, but let the animation
        // communicate the three normal conversation states without a status label.
        SpiLcdDisplay::SetStatus("");
        if (status_label_ != nullptr) {
            DisplayLockGuard lock(this);
            lv_obj_add_flag(status_label_, LV_OBJ_FLAG_HIDDEN);
        }
        SetEmotion(emotion);
    }

    void SetEmotion(const char* emotion) override {
        if (emotion == nullptr) {
            return;
        }

        // Keep the central display board-owned. Normal server emotions (happy,
        // neutral, etc.) must not replace Samantha's current animation.
        if (std::strcmp(emotion, "robot_2") == 0) {
            SpiLcdDisplay::SetEmotion("idle");
        } else if (std::strcmp(emotion, "idle") == 0 || std::strcmp(emotion, "listening") == 0 ||
                   std::strcmp(emotion, "thinking") == 0 || std::strcmp(emotion, "speaking") == 0) {
            SpiLcdDisplay::SetEmotion(emotion);
        }
    }

    void SetChatMessage(const char* role, const char* content) override {
        const char* safe_role = role != nullptr ? role : "system";
        if (std::strcmp(safe_role, "user") == 0) {
            // STT text is a visual state trigger only; do not render the transcript.
            SpiLcdDisplay::ClearChatMessages();
            SetEmotion("thinking");
            return;
        }

        SpiLcdDisplay::SetChatMessage(safe_role, content != nullptr ? content : "");
        if (std::strcmp(safe_role, "assistant") != 0) {
            return;
        }

        DisplayLockGuard lock(this);
        if (bottom_bar_ != nullptr) {
            lv_obj_set_style_bg_color(bottom_bar_, lv_color_hex(0xD1684E), 0);
            lv_obj_set_style_bg_opa(bottom_bar_, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(bottom_bar_, 0, 0);
            lv_obj_set_style_pad_all(bottom_bar_, 0, 0);
        }
        if (chat_message_label_ != nullptr) {
            lv_obj_set_style_text_color(chat_message_label_, lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_text_align(chat_message_label_, LV_TEXT_ALIGN_CENTER, 0);
        }
    }

    void SetHideSubtitle(bool hide) override {
        (void)hide;
        // Samantha's assistant sentence is the conversation UI, so keep it visible.
        SpiLcdDisplay::SetHideSubtitle(false);
    }

    void SetTheme(Theme* theme) override {
        if (theme == nullptr) {
            return;
        }
        ApplySamanthaPalette(theme);
        if (IsSetupUICalled()) {
            SpiLcdDisplay::SetTheme(theme);
        } else {
            Display::SetTheme(theme);
        }
    }

private:
    static void ApplySamanthaPalette(Theme* theme) {
        if (theme == nullptr) {
            return;
        }
        auto* lvgl_theme = static_cast<LvglTheme*>(theme);
        const lv_color_t coral = lv_color_hex(0xD1684E);
        const lv_color_t white = lv_color_hex(0xFFFFFF);
        lvgl_theme->set_background_color(coral);
        lvgl_theme->set_chat_background_color(coral);
        lvgl_theme->set_user_bubble_color(coral);
        lvgl_theme->set_assistant_bubble_color(coral);
        lvgl_theme->set_system_bubble_color(coral);
        lvgl_theme->set_system_text_color(white);
        lvgl_theme->set_text_color(white);
        lvgl_theme->set_border_color(coral);
        lvgl_theme->set_background_image(nullptr);
    }
};

class AiPassportBoard : public WifiBoard {
private:
    i2c_master_bus_handle_t codec_i2c_bus_;
    Button* adc_button_[kAdcButtonNum];
    adc_oneshot_unit_handle_t adc_handle_ = nullptr;
    LcdDisplay* display_;
    Cw2017BatteryMonitor* battery_;

    void InitializeCodecI2c() {
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = I2C_NUM_0,
            .sda_io_num = AUDIO_CODEC_I2C_SDA_PIN,
            .scl_io_num = AUDIO_CODEC_I2C_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {
                .enable_internal_pullup = 1,
            },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &codec_i2c_bus_));

        // CW2017 fuel gauge is optional; a missing chip just disables battery UI.
        battery_ = new Cw2017BatteryMonitor(codec_i2c_bus_, BATTERY_CW2017_ADDR);
        battery_->Initialize();
    }

    void ChangeVolume(int delta) {
        auto codec = GetAudioCodec();
        auto volume = codec->output_volume() + delta;
        if (volume > 100) {
            volume = 100;
        }
        if (volume < 0) {
            volume = 0;
        }
        codec->SetOutputVolume(volume);
        GetDisplay()->ShowNotification(Lang::Strings::VOLUME + std::to_string(volume));
    }

    void ToggleChat() {
        auto& app = Application::GetInstance();
        if (app.GetDeviceState() == kDeviceStateStarting) {
            EnterWifiConfigMode();
            return;
        }
        app.ToggleChatState();
    }

    void InitializeButtons() {
        for (int i = 0; i < kAdcButtonNum; i++) {
            adc_button_[i] = nullptr;
        }

        // One ADC1 unit shared by all three ladder keys. AdcButton reuses the
        // handle when adc_config.adc_handle is non-null, so the same physical
        // pin can decode several keys without "adc1 is already in use".
        adc_oneshot_unit_init_cfg_t init_cfg = {
            .unit_id = ADC_UNIT_1,
        };
        ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc_handle_));

        button_adc_config_t adc_cfg = {};
        adc_cfg.adc_handle = &adc_handle_;
        adc_cfg.unit_id = ADC_UNIT_1;
        adc_cfg.adc_channel = ADC_CHANNEL_0;  // GPIO0

        adc_cfg.button_index = kAdcButtonUp;      // UP:   ~0 mV
        adc_cfg.min = BSP_ADC_BUTTON_UP_MIN;
        adc_cfg.max = BSP_ADC_BUTTON_UP_MAX;
        adc_button_[kAdcButtonUp] = new AdcButton(adc_cfg);

        adc_cfg.button_index = kAdcButtonDown;    // DOWN: ~300 mV
        adc_cfg.min = BSP_ADC_BUTTON_DOWN_MIN;
        adc_cfg.max = BSP_ADC_BUTTON_DOWN_MAX;
        adc_button_[kAdcButtonDown] = new AdcButton(adc_cfg);

        adc_cfg.button_index = kAdcButtonOk;      // OK:   ~595 mV
        adc_cfg.min = BSP_ADC_BUTTON_OK_MIN;
        adc_cfg.max = BSP_ADC_BUTTON_OK_MAX;
        adc_button_[kAdcButtonOk] = new AdcButton(adc_cfg);

        // Button callbacks run on the button task; schedule all UI/audio
        // work onto the main task so LVGL and codec access stay on one thread.
        auto up = adc_button_[kAdcButtonUp];
        up->OnClick([this]() {
            Application::GetInstance().Schedule([this]() { ChangeVolume(10); });
        });
        up->OnLongPress([this]() {
            Application::GetInstance().Schedule([this]() {
                GetAudioCodec()->SetOutputVolume(100);
                GetDisplay()->ShowNotification(Lang::Strings::MAX_VOLUME);
            });
        });

        auto down = adc_button_[kAdcButtonDown];
        down->OnClick([this]() {
            Application::GetInstance().Schedule([this]() { ChangeVolume(-10); });
        });
        down->OnLongPress([this]() {
            Application::GetInstance().Schedule([this]() {
                GetAudioCodec()->SetOutputVolume(0);
                GetDisplay()->ShowNotification(Lang::Strings::MUTED);
            });
        });

        auto ok = adc_button_[kAdcButtonOk];
        ok->OnClick([this]() {
            Application::GetInstance().Schedule([this]() { ToggleChat(); });
        });
    }

    void InitializeSpi() {
        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = DISPLAY_SPI_MOSI_PIN;
        buscfg.miso_io_num = GPIO_NUM_NC;
        buscfg.sclk_io_num = DISPLAY_SPI_SCK_PIN;
        buscfg.quadwp_io_num = GPIO_NUM_NC;
        buscfg.quadhd_io_num = GPIO_NUM_NC;
        buscfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
        ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    }

    void InitializeDisplay() {
        esp_lcd_panel_io_handle_t panel_io = nullptr;
        esp_lcd_panel_handle_t panel = nullptr;

        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num = DISPLAY_SPI_CS_PIN;
        io_config.dc_gpio_num = DISPLAY_DC_PIN;
        io_config.spi_mode = 0;
        io_config.pclk_hz = 40 * 1000 * 1000;
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 8;
        io_config.lcd_param_bits = 8;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &panel_io));

        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = DISPLAY_RST_PIN;  // -1 -> software reset
        panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
        panel_config.bits_per_pixel = 16;
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io, &panel_config, &panel));

        esp_lcd_panel_reset(panel);
        esp_lcd_panel_init(panel);

        // Panel-specific power/porch/gamma sequence for the ST7789P3 module
        // used on the Passport (copied from the original badge firmware).
        static const struct {
            uint8_t command;
            uint8_t data[16];
            uint8_t data_length;
            uint16_t delay_ms;
        } kSt7789P3InitCommands[] = {
            {0xB2, {0x05, 0x05, 0x00, 0x33, 0x33}, 5, 0},
            {0xB7, {0x35}, 1, 0},
            {0xBB, {0x21}, 1, 0},
            {0xC0, {0x2C}, 1, 0},
            {0xC2, {0x01}, 1, 0},
            {0xC3, {0x0B}, 1, 0},
            {0xC4, {0x20}, 1, 0},
            {0xC6, {0x0F}, 1, 0},
            {0xD0, {0xA7, 0xA1}, 2, 0},
            {0xD0, {0xA4, 0xA1}, 2, 0},
            {0xD6, {0xA1}, 1, 0},
            {0xE0, {0xD0, 0x04, 0x08, 0x0A, 0x09, 0x05, 0x2D, 0x43,
                    0x49, 0x09, 0x16, 0x15, 0x26, 0x2B}, 14, 0},
            {0xE1, {0xD0, 0x03, 0x09, 0x0A, 0x0A, 0x06, 0x2E, 0x44,
                    0x40, 0x3A, 0x15, 0x15, 0x26, 0x2A}, 14, 10},
        };
        for (const auto& cmd : kSt7789P3InitCommands) {
            esp_lcd_panel_io_tx_param(panel_io, cmd.command, cmd.data, cmd.data_length);
            if (cmd.delay_ms > 0) {
                vTaskDelay(pdMS_TO_TICKS(cmd.delay_ms));
            }
        }

        esp_lcd_panel_invert_color(panel, DISPLAY_INVERT_COLOR);
        esp_lcd_panel_set_gap(panel, DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y);
        esp_lcd_panel_swap_xy(panel, DISPLAY_SWAP_XY);
        esp_lcd_panel_mirror(panel, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);
        esp_lcd_panel_disp_on_off(panel, true);

        display_ = new SamanthaPassportDisplay(panel_io, panel, DISPLAY_WIDTH, DISPLAY_HEIGHT,
                                               DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y, DISPLAY_MIRROR_X,
                                               DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
    }

public:
    AiPassportBoard() : display_(nullptr), battery_(nullptr) {
        InitializeCodecI2c();
        InitializeSpi();
        InitializeDisplay();
        InitializeButtons();
        GetBacklight()->RestoreBrightness();
    }

    virtual AudioCodec* GetAudioCodec() override {
        static Es8311AudioCodec audio_codec(
            codec_i2c_bus_,
            I2C_NUM_0,
            AUDIO_INPUT_SAMPLE_RATE,
            AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_MCLK,
            AUDIO_I2S_GPIO_BCLK,
            AUDIO_I2S_GPIO_WS,
            AUDIO_I2S_GPIO_DOUT,
            AUDIO_I2S_GPIO_DIN,
            AUDIO_CODEC_PA_PIN,
            AUDIO_CODEC_ES8311_ADDR);
        return &audio_codec;
    }

    virtual Display* GetDisplay() override {
        return display_;
    }

    virtual Backlight* GetBacklight() override {
        static PwmBacklight backlight(DISPLAY_BACKLIGHT_PIN, DISPLAY_BACKLIGHT_OUTPUT_INVERT);
        return &backlight;
    }

    virtual bool GetBatteryLevel(int& level, bool& charging, bool& discharging) override {
        if (!battery_ || !battery_->IsPresent()) {
            return false;
        }
        int soc = battery_->GetBatteryLevel();
        if (soc < 0) {
            return false;
        }
        level = soc;
        // CW2017 reports no charge state and the Passport has no charge-detect
        // GPIO, so report a plain (discharging) reading.
        charging = false;
        discharging = true;
        return true;
    }
};

DECLARE_BOARD(AiPassportBoard);
