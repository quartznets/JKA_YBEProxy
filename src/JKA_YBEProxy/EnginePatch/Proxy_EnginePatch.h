#pragma once

// ==================================================
// INCLUDE
// ==================================================

#include "JKA_YBEProxy/Proxy_Header.h"

// ==================================================
// DEFINE
// ==================================================

// ==================================================
// STRUCTS
// ==================================================



// ==================================================
// EXTERN VARIABLE
// ==================================================

// ==================================================
// FUNCTION
// ==================================================

void Proxy_SV_CalcPings(void);

void (*Original_SV_SendMessageToClient)(msg_t*, client_t*);
void Proxy_SV_SendMessageToClient(msg_t* msg, client_t* client);

void (*Original_SV_UserMove)(client_t*, msg_t*, qboolean);
void Proxy_SV_UserMove(client_t* cl, msg_t* msg, qboolean delta);