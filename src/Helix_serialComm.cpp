#include <Arduino.h>
#include "Helix_Display.h"
#include "Helix_Memory.h"
#include "Helix_serialComm.h"
#include "Helix_Logger.h"
#include "Helix_Wifi.h"
#include <cstdarg>
#include <iostream>


// H_log Serial("Serial Comm");
int incomingByte = 0;
String SerialString;
bool fm_filewriting = false;
const char *fm_filetowrite;
int fm_BUFFER_SIZE;


void Helix_Sercomm_EXEC(String incomingString);

void Sercomm_Printf(const char* fmt...) // C-style "const char* fmt, ..." is also valid
{
    va_list args;
    va_start(args, fmt);
 AJAXTerminal_Send_NOCR(String(*fmt).c_str());
    while (*fmt != '\0')
    {
       

        if (*fmt == 'd')
        {
            int i = va_arg(args, int);
            std::cout << i << '\n';
      
        }
        else if (*fmt == 'c')
        {
            // note automatic conversion to integral type
            int c = va_arg(args, int);
            std::cout << static_cast<char>(c) << '\n';
        }
        else if (*fmt == 'f')
        {
            double d = va_arg(args, double);
            std::cout << d << '\n';
        }
             
        ++fmt;
    }
 
    va_end(args);
}



void Sercomm_Println(String value)
{
    Serial.println(value);
    AJAXTerminal_Send(value.c_str());
}

void Sercomm_Print(String value)
{
    Serial.print(value);
    AJAXTerminal_Send_NOCR(value.c_str());
}

String SC_Tokenizer(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++)
    {
        if (data.charAt(i) == separator || i == maxIndex)
        {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i + 1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void Helix_SerialComm_loop()
{

    // check if data is available
    if (Serial.available() > 0)
    {
        if (!fm_filewriting)
        {
            String incomingString;

            // read the incoming string:
            incomingString = Serial.readStringUntil('\n');

            // prints the received data

            Sercomm_Print("I received: ");
            Sercomm_Println(incomingString);
            incomingString.trim();
            Helix_Sercomm_EXEC(incomingString);
        }

#pragma region FileWriter
        else // File Write statement
        {
            int rxlen = Serial.available();
            if (rxlen > 0)
            {
                int rlen;                   // number of bytes to read
                if (rxlen > fm_BUFFER_SIZE) // check if the data exceeds the buffer size
                    rlen = fm_BUFFER_SIZE;  // if yes, read BUFFER_SIZE bytes. The remaining will be read in the next time
                else
                {
                    rlen = rxlen;

                    // read the incoming bytes:
                    //     rlen = Serial.readBytes(fm_buf, rlen);

                    Sercomm_Print("BUFFSIZE: ");
                    Sercomm_Println((String)fm_BUFFER_SIZE);
                    char fm_buf[fm_BUFFER_SIZE];
                    int rlen = Serial.readBytes(fm_buf, fm_BUFFER_SIZE);

                    // prints the received data
                    // Sercomm_Print("I received: ");
                    // for (int i = 0; i < rlen; i++)
                    // {
                    //     Sercomm_Print(fm_buf[i]);
                    // }
                    // sc_writefile(fm_filetowrite, fm_buf);
                    sc_writefile("/test2.txt", fm_buf);
                    fm_filewriting = false;
                    rxlen = 0;
                }
            }
        }

#pragma endregion
    }
}

void Helix_Sercomm_EXEC(String incomingString)
{

#pragma region Help
    if (incomingString == "help")
    {
        Sercomm_Println("type:");
        Sercomm_Print("");
        Sercomm_Println("-             COMMONS Commands                 -");
        Sercomm_Println("-   reboot                                      to restart ESP");
        Sercomm_Println("-   lvlog                                       to toggle lvgl log");
        Sercomm_Println("-   log_level   [level]                         to change log priority [0-4]");
        Sercomm_Println("-   factory                                     to delete settings and reboot");
        Sercomm_Print("");
        Sercomm_Println("-           FILE MANAGER Commands              -");
        Sercomm_Println("-   fm_filelist                                 to list all files in directory");
        Sercomm_Println("-   fm_mkdir    [/path]                         to create directory");
        Sercomm_Println("-   fm_rmdir    [/path]                         to remove directory");
        Sercomm_Println("-   fm_rmfile   [/path]                         to remove file");
        Sercomm_Println("-   fm_write    [/path] [buffer size (int)]     to write file");
        Sercomm_Print("");
        Sercomm_Println("-         SETTINGS MANAGER Commands            -");
        Sercomm_Println("-   settings_list                               to list all settings");
        Sercomm_Println("-   settings_write    [key] [value]             to change setting");
        Sercomm_Println("-   RAW_settings_write[key] [value]             to change setting (WARNING NO TYPE CONTROL ---DEPRECATED--- USE ONLY FOR STRING!!!)");
        Sercomm_Print("");
        Sercomm_Println("-                    AJAX Injector              -");
        Sercomm_Println("-   AJAX_Settings    [key] [value]              to send AJAX message on setting page");
        Sercomm_Println("-   AJAX_TerminalTEST   [message]               to send AJAX message on setting page");
    }
#pragma endregion

#pragma region CommonCommands

    else if (incomingString == "reboot")
    {
        Sercomm_Println("Restarting Helix Control Panel");
        delay(3000);
        ESP.restart();
    }
    else if (incomingString == "factory")
    {
        Helix_Factory_Reset();
    }
    else if (incomingString == "lvlog")
    {
        Sercomm_Println("toggle log");
        if (LVGL_Log)
        {
            LVGL_Log = false;
            Sercomm_Println("LV_log: FALSE");
        }
        else
        {
            LVGL_Log = true;
            Sercomm_Println("LV_log: TRUE");
        }
    }
    else if (incomingString.indexOf("log_level") >= 0)
    {
        String level = SC_Tokenizer(incomingString, ' ', 1);
        level.trim();
        Helix_LOG_Priority = level.toInt();
    }

#pragma endregion

#pragma region Filemanager

    else if (incomingString.indexOf("fm_filelist") >= 0)
    {
        sc_filelist();
    }
    else if (incomingString.indexOf("fm_mkdir") >= 0)
    {
        String path = SC_Tokenizer(incomingString, ' ', 1);
        path.trim();
        if (path.isEmpty())
        { // bail if we didn't get anything
            Sercomm_Println("EMPTY PATH.. ABORT");
        }
        else
        {
            if (path.substring(0, 1) != "/")
            {
                Sercomm_Println("the path does not start with '/' ABORT");
            }
            else
            {
                sc_mkdir(path.c_str());
            }
        }
    }
    else if (incomingString.indexOf("fm_rmdir") >= 0)
    {
        String path = SC_Tokenizer(incomingString, ' ', 1);
        path.trim();
        if (path.isEmpty())
        { // bail if we didn't get anything
            Sercomm_Println("EMPTY PATH.. ABORT");
        }
        else
        {
            if (path.substring(0, 1) != "/")
            {
                Sercomm_Println("the path does not start with '/' ABORT");
            }
            else
            {
                sc_rmdir(path.c_str());
            }
        }
    }
    else if (incomingString.indexOf("fm_rmfile") >= 0)
    {
        String path = SC_Tokenizer(incomingString, ' ', 1);
        path.trim();
        if (path.isEmpty())
        { // bail if we didn't get anything
            Sercomm_Println("EMPTY PATH.. ABORT");
        }
        else
        {
            if (path.substring(0, 1) != "/")
            {
                Sercomm_Println("the path does not start with '/' ABORT");
            }
            else
            {
                sc_rmfile(path.c_str());
            }
        }
    }
    else if (incomingString.indexOf("fm_write") >= 0)
    {

        String path = SC_Tokenizer(incomingString, ' ', 1);
        String buf = SC_Tokenizer(incomingString, ' ', 2);
        //  path.trim();
        buf.trim();
        if (path.isEmpty())
        { // bail if we didn't get anything
            Sercomm_Println("EMPTY PATH.. ABORT");
        }
        else
        {
            if (path.substring(0, 1) != "/")
            {
                Sercomm_Println("the path does not start with '/' ABORT");
                Sercomm_Println(path);
            }
            else
            {
                if (buf.isEmpty())
                {
                    Sercomm_Println("EMPTY Buffer size.. ABORT");
                }
                else
                {
                    if (buf.toInt() <= 0)
                    {
                        Sercomm_Println("Small Buffer size, or wrong set \n maybe non integer value? \n ABORT");
                    }
                    else
                    {
                        fm_filewriting = true;
                        fm_filetowrite = path.c_str();
                        fm_BUFFER_SIZE = buf.toInt();
                        Sercomm_Print("Ready to write: ");
                        Sercomm_Println(fm_filetowrite);
                        Sercomm_Print("Buffer size: ");
                        Sercomm_Println((String)fm_BUFFER_SIZE);
                        Sercomm_Println("Waiting input...");
                    }
                }
            }
        }
    }
#pragma endregion

#pragma region Settingsmanager

    else if (incomingString == "settings_list")
    {
        sc_Settings_List();
    }
    else if (incomingString.indexOf("settings_write") >= 0)
    {
        String key = SC_Tokenizer(incomingString, ' ', 1);
        key.trim();
        String val = SC_Tokenizer(incomingString, ' ', 2);
        val.trim();
        sc_Settings_write(key, val);
    }
    else if (incomingString.indexOf("RAW_settings_write") >= 0)
    {
        String key = SC_Tokenizer(incomingString, ' ', 1);
        key.trim();
        String val = SC_Tokenizer(incomingString, ' ', 2);
        val.trim();
        sc_RAW_Settings_write(key, val);
    }
#pragma endregion

#pragma region AJAX_Settings
    else if (incomingString.indexOf("AJAX_Settings") >= 0)
    {
        String key = SC_Tokenizer(incomingString, ' ', 1);
        key.trim();
        String val = SC_Tokenizer(incomingString, ' ', 2);
        val.trim();

        ajax_settings_send(val.c_str(), key.c_str());
    }

    else if (incomingString.indexOf("AJAX_TerminalTEST") >= 0)
    {
        String MSG = SC_Tokenizer(incomingString, ' ', 1);
        MSG.trim();
        AJAXTerminal_Send(MSG.c_str());
    }

#pragma endregion
}