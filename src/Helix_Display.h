#ifndef _Helix_Display_h_
#define _Helix_Display_h_

extern void Helix_Display_init();
extern void Helix_AP_MODE_Display_init();
extern void Helix_Display_RSOD(String RSOD_TEXT);
extern void Helix_Display_BSOD(String BSOD_TEXT);
extern void Display_Activity_CB(bool Forced = false);
extern void AP_Display_Activity_CB();
extern bool LVGL_Log;
extern bool GUI_ON;
#endif