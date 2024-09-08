#ifndef _Helix_Sensors_h_
#define _Helix_Sensors_h_

extern void Helix_Sensors_init();
extern void Set_Temp(bool up,float NewSetPoint = NULL);
extern void Helix_Sensors_loop();
extern float TEMP;
extern float HUMIDITY;
#endif