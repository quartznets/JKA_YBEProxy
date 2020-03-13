// ==================================================
// INCLUDE
// ==================================================

#include "Proxy_Header.h"
#include "Proxy_Shell.h"

// ==================================================
// DEFINE
// ==================================================

// ==================================================
// TYPEDEF
// ==================================================

// ==================================================
// STRUCT
// ==================================================

// ==================================================
// GLOBALE VARIABLE
// ==================================================

// WIP
unsigned char* pSV_SendMessageToClient;
unsigned char* pSV_UserMove;
unsigned char* pSV_CalcPings;

// ==================================================
// FUNCTION
// ==================================================

void* Proxy_EnginePatch_PingFix_SV_SendMessageToClient(void);
void* Proxy_EnginePatch_PingFix_SV_UserMove(void);
void* Proxy_EnginePatch_DisplaySnapShots_SV_CalcPings(void);