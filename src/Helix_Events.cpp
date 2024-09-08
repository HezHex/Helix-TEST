#include <Arduino.h>
#include "ui.h"
#include "Helix_Sensors.h"
#include "Helix_Memory.h"
#include "Helix_Display.h"
#include "Helix_Wifi.h"

bool FRESET_Mbox_EXISTS = false;

#pragma region Events_Callbacks
void BTDownTemp_Click(lv_event_t *e) { Set_Temp(false); }                    // Down Temp Button Click callback
void BTUpTemp_Click(lv_event_t *e) { Set_Temp(true); }                       // Up Temp Button click callback
void Screen_LowLight(lv_event_t *e) { ledcWrite(0, Screen_OFF_Brightness); } // Screen fade out backlight control callback
void Screen_Settings_Loaded(lv_event_t *e) { Display_Activity_CB(true); }        // call DAC when load setting screen to update fade out delay
void Web_FileManager_Toggle(lv_event_t *e)                                   // WebFileman_Enabled Toggle
{
    Fileman_Enabled = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
    Helix_Update_Settings();
}
void Web_ADV_Settings_Toggle(lv_event_t *e)                                    // WebADVSettings_Enabled Toggle
{
    ADV_WebSettings_Enabled = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
    Helix_Update_Settings();
}
void Web_Terminal_Toggle(lv_event_t *e)                                         // WebTerminal_Enabled Toggle
{
    WebTerminal_Enabled = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
    Helix_Update_Settings();
}

static void FResetMbox_event_cb(lv_event_t * e)                                 //factory reset messagebox callback
{
    lv_obj_t * obj = lv_event_get_current_target(e);
    if(lv_msgbox_get_active_btn(obj) == 1) {
        lv_msgbox_close(obj);
        FRESET_Mbox_EXISTS = false;
    }
    else {
          lv_msgbox_close(obj);
          Helix_Factory_Reset();
    }
}

lv_obj_t * FResetMbox;                                                          //factory reset bessagebox object

void BTFactoryReset_Click(lv_event_t *e) {                                      //factory Reset button click Callback
 static const char * btns[] ={"Reset", "Abort",""};
 FResetMbox = lv_msgbox_create(NULL, "Warning!", "Are you shure you want perform a factory reset?", btns, true);
    lv_obj_add_event_cb(FResetMbox, FResetMbox_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_center(FResetMbox);
FRESET_Mbox_EXISTS = true;
}

void APMode_Toggle(lv_event_t *e)                                         // WebTerminal_Enabled Toggle
{
   if( lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED)) { Helix_AP_init();}

}



#pragma endregion




void Helix_Events_Init()
{
#pragma region Events_Assign
    lv_obj_add_event_cb(ui_BTDownTemp, BTDownTemp_Click, LV_EVENT_CLICKED, NULL);                    // Down Temp button Click event
    lv_obj_add_event_cb(ui_BTUpTemp, BTUpTemp_Click, LV_EVENT_CLICKED, NULL);                        // Up Temp button Click event
    lv_obj_add_event_cb(ui_SCREENIdle, Screen_LowLight, LV_EVENT_SCREEN_LOADED, NULL);               // Screen fade out backlight control event
    lv_obj_add_event_cb(ui_SCREENConnections, Screen_Settings_Loaded, LV_EVENT_SCREEN_LOADED, NULL); // Settings screen on load event
    lv_obj_add_event_cb(ui_SWWebFileManager, Web_FileManager_Toggle, LV_EVENT_VALUE_CHANGED, NULL);  // Webfileman_Enabled Toggle
    lv_obj_add_event_cb(ui_SWWebAdvSettings, Web_ADV_Settings_Toggle, LV_EVENT_VALUE_CHANGED, NULL); // WebADVSettings_Enabled Toggle
    lv_obj_add_event_cb(ui_SWWebTerminal, Web_Terminal_Toggle, LV_EVENT_VALUE_CHANGED, NULL);        // WebTerminal_Enabled Toggle
    lv_obj_add_event_cb(ui_BTFactoryReset, BTFactoryReset_Click, LV_EVENT_CLICKED, NULL);            // Factory reset button
    lv_obj_add_event_cb(ui_SWAccessPoint, APMode_Toggle, LV_EVENT_VALUE_CHANGED, NULL);        // WebTerminal_Enabled Toggle
#pragma endregion





#pragma region ON_Gui_Start

    if (Fileman_Enabled)
    {
        lv_obj_clear_state(ui_SWWebFileManager, LV_STATE_CHECKED);
        lv_obj_add_state(ui_SWWebFileManager, LV_STATE_CHECKED);
    }
    if (ADV_WebSettings_Enabled)
    {
        lv_obj_clear_state(ui_SWWebAdvSettings, LV_STATE_CHECKED);
        lv_obj_add_state(ui_SWWebAdvSettings, LV_STATE_CHECKED);
    }

    if (WebTerminal_Enabled)
    {
        lv_obj_clear_state(ui_SWWebTerminal, LV_STATE_CHECKED);
        lv_obj_add_state(ui_SWWebTerminal, LV_STATE_CHECKED);
    }

    
    lv_label_set_text(ui_LABConnVer, SW_Version.c_str());
    lv_label_set_text(ui_LABConnVer2, SW_Version.c_str());

#pragma endregion

}
