// This file was generated by SquareLine Studio
// SquareLine Studio version: SquareLine Studio 1.4.0
// LVGL version: 8.3.6
// Project name: Helix Control panel 3

#include "../ui.h"

void ui_SCREENIdle_screen_init(void)
{
    ui_SCREENIdle = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_SCREENIdle, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_SCREENIdle, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_SCREENIdle, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_LABIdleTime = lv_label_create(ui_SCREENIdle);
    lv_obj_set_width(ui_LABIdleTime, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_LABIdleTime, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_LABIdleTime, lv_pct(0));
    lv_obj_set_y(ui_LABIdleTime, lv_pct(-40));
    lv_obj_set_align(ui_LABIdleTime, LV_ALIGN_CENTER);
    lv_label_set_text(ui_LABIdleTime, "10:10");
    lv_label_set_recolor(ui_LABIdleTime, "true");
    lv_obj_set_style_text_color(ui_LABIdleTime, lv_color_hex(0x585858), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_LABIdleTime, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_LABIdleTime, &ui_font_MicrogrammaSMALL16, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_LABIdleTemp = lv_label_create(ui_SCREENIdle);
    lv_obj_set_width(ui_LABIdleTemp, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_LABIdleTemp, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_LABIdleTemp, LV_ALIGN_CENTER);
    lv_label_set_text(ui_LABIdleTemp, "20°");
    lv_obj_set_style_text_color(ui_LABIdleTemp, lv_color_hex(0x585858), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_LABIdleTemp, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_LABIdleTemp, &ui_font_NebulaBIG, LV_PART_MAIN | LV_STATE_DEFAULT);


}
