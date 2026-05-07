#pragma once

#include "display/lcd_display.h"

/**
 * @brief Lớp hiển thị biểu cảm GIF cho Robot Otto
 * Kế thừa SpiLcdDisplay, thêm hỗ trợ biểu cảm GIF thông qua EmojiCollection
 */
class OttoEmojiDisplay : public SpiLcdDisplay {
   public:
    /**
     * @brief Hàm khởi tạo, tham số giống SpiLcdDisplay
     */
    OttoEmojiDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel, int width, int height, int offset_x, int offset_y, bool mirror_x, bool mirror_y, bool swap_xy);

    virtual ~OttoEmojiDisplay() = default;
    virtual void SetStatus(const char* status) override;
    virtual void SetPreviewImage(std::unique_ptr<LvglImage> image) override;
    virtual void SetupUI() override;

   private:
    void InitializeOttoEmojis();
    void SetupPreviewImage();
};