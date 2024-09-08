// This file was generated by SquareLine Studio
// SquareLine Studio version: SquareLine Studio 1.4.0
// LVGL version: 8.3.6
// Project name: Helix Control panel 3

#ifndef _HELIX_CONTROL_PANEL_3_UI_H

#define _HELIX_CONTROL_PANEL_3_UI_H



#ifdef __cplusplus
extern "C" {

#endif


#include "lvgl.h"


#include "ui_helpers.h"

#include "ui_events.h"



// SCREEN: ui_SCREENIdle

void ui_SCREENIdle_screen_init(void);

extern lv_obj_t * ui_SCREENIdle;

extern lv_obj_t * ui_LABIdleTime;

extern lv_obj_t * ui_LABIdleTemp;


// SCREEN: ui_SCREENmain

void ui_SCREENmain_screen_init(void);

void ui_event_SCREENmain(lv_event_t * e);

extern lv_obj_t * ui_SCREENmain;

extern lv_obj_t * ui_BTDownTemp;

extern lv_obj_t * ui_BTUpTemp;

extern lv_obj_t * ui_LABTime;

extern lv_obj_t * ui_LABRoomTemp;

extern lv_obj_t * ui_LABRoomSetpoint;

extern lv_obj_t * ui_LABRoomHum;

extern lv_obj_t * ui_LABIndicators;

// SCREEN: ui_SCREENConnections

void ui_SCREENConnections_screen_init(void);

void ui_event_SCREENConnections(lv_event_t * e);

extern lv_obj_t * ui_SCREENConnections;

extern lv_obj_t * ui_PANELConnection;

extern lv_obj_t * ui_IPLab;

extern lv_obj_t * ui_QR;

extern lv_obj_t * ui_SWAccessPoint;

extern lv_obj_t * ui_LABAccessPoint;

extern lv_obj_t * ui_LABConnectionDesc;

extern lv_obj_t * ui_LABConnVer;

// SCREEN: ui_SCREENSettings

void ui_SCREENSettings_screen_init(void);

void ui_event_SCREENSettings(lv_event_t * e);

extern lv_obj_t * ui_SCREENSettings;

extern lv_obj_t * ui_PANELSettings;

extern lv_obj_t * ui_SWWebFileManager;

extern lv_obj_t * ui_LABWebFileManager;

extern lv_obj_t * ui_SWWebAdvSettings;

extern lv_obj_t * ui_LABWebAdvSettings;

extern lv_obj_t * ui_SWWebTerminal;

extern lv_obj_t * ui_LABWebTerminal;

extern lv_obj_t * ui_LABSettingsDesc;

extern lv_obj_t * ui_LABConnVer2;

extern lv_obj_t * ui_BTFactoryReset;

extern lv_obj_t * ui_LABFactoryReset;

extern lv_obj_t * ui____initial_actions0;





LV_IMG_DECLARE(ui_img_hex_background_png);    // assets/HEX_Background.png

LV_IMG_DECLARE(ui_img_tdrelased_png);    // assets/TDrelased.png

LV_IMG_DECLARE(ui_img_tdpressed_png);    // assets/TDpressed.png

LV_IMG_DECLARE(ui_img_turelased_png);    // assets/TUrelased.png

LV_IMG_DECLARE(ui_img_tupressed_png);    // assets/TUpressed.png







LV_FONT_DECLARE(ui_font_MicrogrammaSMALL16);

LV_FONT_DECLARE(ui_font_NebulaBIG);







void ui_init(void);



#ifdef __cplusplus

} /*extern "C"*/

#endif



#endif

