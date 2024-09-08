#ifndef _Helix_Wifi_h_
#define _Helix_Wifi_h_

extern void Helix_Wifi_init();
extern void Helix_AP_init();
extern void printLocalTime();
extern void ajax_event(const char *message, const char *event, uint32_t id);
extern void ajax_settings_send(const char *message, const char *event);
extern void AJAXTerminal_Send(const char *message);
extern void AJAXTerminal_Send_NOCR(const char *message);
extern void HtmlNets();

extern bool WiFi_Error;
extern bool Wifi_Connected;
extern bool Fileman_Enabled ;
extern String WebDateTime;

#endif