#include <Arduino.h>
#include <Helix_Memory.h>
#include <Helix_Logger.h>
#include <Helix_WiFi.h>
#include "ui.h"

#define IND_RED "#ff0000 "
#define IND_GREEN "#00FF00 "
#define IND_GREY "#555555 "
#define IND_BLUE "#00BFFF "
#define IND_END "# "

H_log LI("INDICATORS");

String OLD_Indicator_Result;

bool Force_WARNING = false;

void GUI_Indicator_Update(void *ptr);
void Helix_Indicators_Loop(void *pvParameter);
void GUI_IndicatorCB();
void GUI_Indicators_Time(String Hours, String Minutes);

void Helix_Indicators_Init()
{
    LI.println("indicatorsLoop Allocated", 1);
    LI.println("Initialized", 1);
}

void GUI_IndicatorCB()
{

    bool warning_state = Force_WARNING;
    String warning;
    String memory;
    String wifi = IND_GREEN LV_SYMBOL_WIFI IND_END;
    String mqtt = IND_GREY LV_SYMBOL_LOOP IND_END;
    String AP = IND_BLUE "AP" IND_END;

    if (Helix_Memory_State)
    {
   
        if (Helix_Memory_Writing)
        {
            memory = IND_BLUE LV_SYMBOL_SD_CARD IND_END;
        }
        else {
                 memory = IND_GREY LV_SYMBOL_SD_CARD IND_END;
        }
    }
    else
    {
        memory = IND_RED LV_SYMBOL_SD_CARD IND_END;
        warning_state = true;
    }

    if (warning_state)
    {
        warning = IND_RED LV_SYMBOL_WARNING IND_END;
    }
    else
    {
        warning = IND_GREY LV_SYMBOL_WARNING IND_END;
    }

    if (Wifi_Connected)
    {
       wifi = IND_GREEN LV_SYMBOL_WIFI IND_END;
       WiFi_Error = false;
    }
    else
    {
        wifi = IND_GREY LV_SYMBOL_WIFI IND_END;
    }

    if (WiFi_Error)
    {
       wifi = IND_RED LV_SYMBOL_WIFI IND_END;
       GUI_Indicators_Time("","");
    }


  //  if (AP_Mode)
  //  {
  //    AP = IND_BLUE "AP" IND_END;
  //    GUI_Indicators_Time("","");
  //  }
    else
    {
       AP = IND_GREY "AP" IND_END;
    }

    String Indicator_Result = warning + memory + mqtt + wifi + AP;

    if (OLD_Indicator_Result != Indicator_Result)
    {

        OLD_Indicator_Result = Indicator_Result;

   lv_async_call(GUI_Indicator_Update, NULL);
    }
}

void GUI_Indicator_Update(void *ptr)
{
    if (lv_obj_is_valid(ui_LABIndicators))
    {
        lv_label_set_text(ui_LABIndicators, OLD_Indicator_Result.c_str());
    }
}



void GUI_Indicators_Time(String Hours, String Minutes) {

if(Hours.length() <= 1) {
    Hours = "0" + Hours;
}
if(Minutes.length() <= 1) {
    Minutes = "0" + Minutes;
}
    String Time = Hours + ":" + Minutes;

   if (WiFi_Error) //  if (WiFi_Error || AP_Mode)
       Time = IND_RED "0000:0000" IND_END;
   if (lv_obj_is_valid(ui_LABIdleTime))
   {
     lv_label_set_text(ui_LABIdleTime, Time.c_str());
   }
   if (lv_obj_is_valid(ui_LABTime))
   {
     lv_label_set_text(ui_LABTime, Time.c_str());
   }

}


void GUI_Indicators_QrCode(String QRCODE_IP_OR_APSSID = "127.0.0.1", bool Ap_Mode = false) {
if(Ap_Mode) {
    
       String data = "WIFI:S:";
       data += QRCODE_IP_OR_APSSID;
       data += ";T:nopass;P:HELIX;H:false;;";
       lv_qrcode_update(ui_QR, data.c_str(), strlen(data.c_str()));       // change qrcode image
       String stringbuilder = "Connect to SSID \n" + QRCODE_IP_OR_APSSID + " \nor scan qrcode";     // Build Label text
       lv_label_set_text(ui_IPLab, stringbuilder.c_str());                // Update Label text
       /// make ap mode indicators turn on blue led and set qrcode and label
}
else {
       // UPDATE QRCODE & IP LABEL
      String data = "http://";
      data += QRCODE_IP_OR_APSSID + "/";                                 // internal ip address
      lv_qrcode_update(ui_QR, data.c_str(), strlen(data.c_str()));       // change qrcode image
      String stringbuilder = "Navigate \n" + data + " \nor scan qrcode";     // Build Label text
      lv_label_set_text(ui_IPLab, stringbuilder.c_str());                // Update Label text
      // END UPDATE
}
}