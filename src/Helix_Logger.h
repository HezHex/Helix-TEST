
#ifndef _Helix_Logger_h_
#define _Helix_Logger_h_

#include "ui.h"
#include <Arduino.h>
#include "Helix_Memory.h"
#include "Helix_WiFi.h"


class H_log
{                  // The class
public:            // Access specifier
    String module; // Attribute
                   // int priority;
    bool println_wait = false;

    H_log(String Module)
    { // Constructor with parameters
        module = Module;
    }

    const char *H_Prefix(String message)
    {
       // message = WebDateTime + message;
        String ret = "[" + module + "]      " + message;
        return ret.c_str();
    }

    void print(String message, int Priority = 0)
    {
      //  message = WebDateTime + message;

        if (Helix_LOG_Priority <= Priority)
        {
            if (!println_wait)
            {
                Serial.print("[" + module + "]      ");
            }
            println_wait = true;
            Serial.print(message);
            
             AJAXTerminal_Send(message.c_str());
        }
        Helix_Gui_Log_Update(message, Priority);
    }
    void println(String message, int Priority = 0)
    {
     //   message = WebDateTime + message;
        if (Helix_LOG_Priority <= Priority)
        {
            if (!println_wait)
            {
                Serial.print("[" + module + "]      ");
            }
            else
            {
                println_wait = false;
            }
            Serial.println(message);
              AJAXTerminal_Send(message.c_str());
        }
        Helix_Gui_Log_Update(message, Priority);
    }

    void check_Mem()
    {
        uint32_t freeHeapBytes = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
        uint32_t totalHeapBytes = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
        float percentageHeapFree = freeHeapBytes * 100.0f / (float)totalHeapBytes;
        // Print to serial
        Serial.printf("[Memory]      %.1f%% free - %d of %d bytes free [ %s ]\n", percentageHeapFree, freeHeapBytes, totalHeapBytes, module);
    }

private:
    void Helix_Gui_Log_Update(String message, int Priority)
    {
        if (Priority >= Helix_LOG_Priority)
        {
            //    if (lv_obj_is_valid(ui_LogTextArea))
            //    {
            //        lv_textarea_add_text(ui_LogTextArea, message.c_str());
            //        lv_textarea_add_text(ui_LogTextArea,"\n");
            //    }
        }
    }
};
#endif
