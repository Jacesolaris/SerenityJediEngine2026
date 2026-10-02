/*
===========================================================================
Pazaak - multiplayer host (see g_pazaak.c)
===========================================================================
*/

#ifndef G_PAZAAK_H
#define G_PAZAAK_H

void G_Pazaak_Init(void);
void G_Pazaak_Shutdown(void);
void G_Pazaak_RunFrame(void);
void G_Pazaak_ClientDisconnect(int clientNum);
qboolean G_Pazaak_ClientCommand(gentity_t* ent, const char* cmd);
qboolean G_Pazaak_IsPlaying(int clientNum);

#endif // G_PAZAAK_H
