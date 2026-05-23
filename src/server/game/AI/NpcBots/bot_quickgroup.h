#ifndef BOT_QUICKGROUP_H
#define BOT_QUICKGROUP_H

class ChatHandler;
class Player;

namespace BotQuickGroup
{
bool Fill(Player* player, uint8 partySize, uint8 tanksNeeded, uint8 offTanksNeeded, uint8 healersNeeded, uint8 dpsNeeded);

bool Handle5(Player* player);
bool Handle10(Player* player);
bool Handle25(Player* player);
bool Handle40(Player* player);
bool HandleDissolve(Player* player);

bool Handle5Cmd(ChatHandler* handler, char const* args);
bool Handle10Cmd(ChatHandler* handler, char const* args);
bool Handle25Cmd(ChatHandler* handler, char const* args);
bool Handle40Cmd(ChatHandler* handler, char const* args);
bool HandleDissolveCmd(ChatHandler* handler, char const* args);
}

#endif
