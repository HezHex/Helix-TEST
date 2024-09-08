#ifndef _Helix_serialComm_h_
#define _Helix_serialComm_h_

extern void Helix_SerialComm_loop();
extern void Helix_Sercomm_EXEC(String incomingString);
extern void Sercomm_Println(String value);
extern void Sercomm_Print(String value);
extern void Sercomm_Printf(const char* fmt...);
#endif