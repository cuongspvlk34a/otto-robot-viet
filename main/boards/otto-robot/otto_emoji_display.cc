#include "otto_emoji_display.h"

#include <esp_log.h>

#include <cstring>
#include <vector>

#include "assets.h"
#include "assets/lang_config.h"
#include "display/lvgl_display/emoji_collection.h"
#include "display/lvgl_display/lvgl_image.h"
#include "display/lvgl_display/lvgl_theme.h"

#define TAG "OttoEmojiDisplay"
OttoEmojiDisplay::OttoEmojiDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel, int width, int height, int offset_x, int offset_y, bool mirror_x, bool mirror_y, bool swap_xy)
    : SpiLcdDisplay(panel_io, panel, width, height, offset_x, offset_y, mirror_x, mirror_y, swap_xy) {
}

void OttoEmojiDisplay::SetupUI() {
    // Ngăn gọi lặp - hàm SetupUI() cha cũng kiểm tra, nhưng kiểm tra sớm ở đây để thoát nhanh
    if (setup_ui_called_) {
        ESP_LOGW(TAG, "SetupUI() được gọi nhiều lần, bỏ qua lần gọi trùng");
        return;
    }
    
    // Gọi SetupUI() cha trước để tạo toàn bộ đối tượng LVGL
    SpiLcdDisplay::SetupUI();
    
    // Khởi tạo ảnh xem trước sau khi UI đã sẵn sàng - giải phóng lock trước khi gọi SetEmotion
    // để tránh deadlock (SetEmotion cũng tự lấy DisplayLockGuard bên trong)
    {
        DisplayLockGuard lock(this);
        lv_obj_set_size(preview_image_, width_ , height_ );
    }

    // Đặt cảm xúc mặc định sau khi UI đã được khởi tạo
    SetEmotion("staticstate");
}

void OttoEmojiDisplay::SetupPreviewImage() {
    DisplayLockGuard lock(this);
    if (preview_image_ == nullptr) {
        ESP_LOGW(TAG, "SetupPreviewImage được gọi nhưng preview_image_ là nullptr (UI chưa khởi tạo)");
        return;
    }
    lv_obj_set_size(preview_image_, width_ , height_ );
}

void OttoEmojiDisplay::InitializeOttoEmojis() {
    ESP_LOGI(TAG, "Khởi tạo biểu cảm Otto sẽ được xử lý bởi hệ thống Assets");
    // Khởi tạo biểu cảm đã chuyển sang hệ thống assets, cấu hình qua DEFAULT_EMOJI_COLLECTION=otto-gif
    // assets.cc sẽ tải biểu cảm GIF từ phân vùng assets và áp dụng vào theme
    // Lưu ý: Cảm xúc mặc định hiện được đặt trong SetupUI() sau khi đối tượng LVGL được tạo
}

LV_FONT_DECLARE(OTTO_ICON_FONT);
void OttoEmojiDisplay::SetStatus(const char* status) {
    auto lvgl_theme = static_cast<LvglTheme*>(current_theme_);
    auto text_font = lvgl_theme->text_font()->font();
    DisplayLockGuard lock(this);
    if (!status) {
        ESP_LOGE(TAG, "SetStatus: status là nullptr");
        return;
    }

    if (strcmp(status, Lang::Strings::LISTENING) == 0) {
        lv_obj_set_style_text_font(status_label_, &OTTO_ICON_FONT, 0);
        lv_label_set_text(status_label_, "\xEF\x84\xB0");  // U+F130 biểu tượng micro
        lv_obj_clear_flag(status_label_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(network_label_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(battery_label_, LV_OBJ_FLAG_HIDDEN);
        return;
    } else if (strcmp(status, Lang::Strings::SPEAKING) == 0) {
        lv_obj_set_style_text_font(status_label_, &OTTO_ICON_FONT, 0);
        lv_label_set_text(status_label_, "\xEF\x80\xA8");  // U+F028 biểu tượng loa/nói
        lv_obj_clear_flag(status_label_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(network_label_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(battery_label_, LV_OBJ_FLAG_HIDDEN);
        return;
    } else if (strcmp(status, Lang::Strings::CONNECTING) == 0) {
        lv_obj_set_style_text_font(status_label_, &OTTO_ICON_FONT, 0);
        lv_label_set_text(status_label_, "\xEF\x83\x81");  // U+F0c1 biểu tượng kết nối
        lv_obj_clear_flag(status_label_, LV_OBJ_FLAG_HIDDEN);
        return;
    } else if (strcmp(status, Lang::Strings::STANDBY) == 0) {
        lv_obj_set_style_text_font(status_label_, text_font, 0);
        lv_label_set_text(status_label_, "");
        lv_obj_clear_flag(status_label_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(network_label_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(battery_label_, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_obj_set_style_text_font(status_label_, text_font, 0);
    lv_label_set_text(status_label_, status);
}

void OttoEmojiDisplay::SetPreviewImage(std::unique_ptr<LvglImage> image) {
    DisplayLockGuard lock(this);
    if (preview_image_ == nullptr) {
        ESP_LOGE(TAG, "Ảnh xem trước chưa được khởi tạo");
        return;
    }

    if (image == nullptr) {
        esp_timer_stop(preview_timer_);
        lv_obj_remove_flag(emoji_box_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(preview_image_, LV_OBJ_FLAG_HIDDEN);
        preview_image_cached_.reset();
        if (gif_controller_) {
            gif_controller_->Start();
        }
        return;
    }

    preview_image_cached_ = std::move(image);
    auto img_dsc = preview_image_cached_->image_dsc();
    // Đặt nguồn ảnh và hiển thị ảnh xem trước
    lv_image_set_src(preview_image_, img_dsc);
    lv_image_set_rotation(preview_image_, 900);
    if (img_dsc->header.w > 0 && img_dsc->header.h > 0) {
        // hệ số thu phóng 1.0
        lv_image_set_scale(preview_image_, 256 * width_ / img_dsc->header.w);
    }

    // Ẩn emoji_box_
    if (gif_controller_) {
        gif_controller_->Stop();
    }
    lv_obj_add_flag(emoji_box_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(preview_image_, LV_OBJ_FLAG_HIDDEN);
    esp_timer_stop(preview_timer_);
    ESP_ERROR_CHECK(esp_timer_start_once(preview_timer_, PREVIEW_IMAGE_DURATION_MS * 1000));
}