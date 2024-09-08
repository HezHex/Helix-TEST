#ifndef _Helix_Indicators_h_
#define _Helix_Indicators_h_
extern void Helix_Indicators_Init();
extern void GUI_IndicatorCB();
extern bool Force_WARNING;
extern void GUI_Indicators_QrCode(String QRCODE_IP = "127.0.0.1", bool Ap_Mode = false);
extern void GUI_Indicators_Time(String Hours, String Minutes);
#endif
