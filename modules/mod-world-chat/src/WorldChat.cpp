/*
<--------------------------------------------------------------------------->
- Developer(s): WiiZZy
- Complete: 100%
- ScriptName: 'World chat'
- Comment: Fully tested
<--------------------------------------------------------------------------->
*/

#include "ScriptObject.h"
#include "Log.h"
#include "Player.h"
#include "Channel.h"
#include "Chat.h"
#include "Common.h"
#include "World.h"
#include "WorldSession.h"
#include "Config.h"
#include <unordered_map>

/* VERSION */
float ver = 2.0f;

/* Colors */
constexpr auto WORLD_CHAT_ALLIANCE_BLUE = "|cff3399FF";
constexpr auto WORLD_CHAT_HORDE_RED = "|cffCC0000";
constexpr auto WORLD_CHAT_WHITE = "|cffFFFFFF";
constexpr auto WORLD_CHAT_GREEN = "|cff00CC00";
constexpr auto WORLD_CHAT_RED = "|cffFF0000";
constexpr auto WORLD_CHAT_BLUE = "|cff6666FF";
constexpr auto WORLD_CHAT_BLACK = "|cff000000";
constexpr auto WORLD_CHAT_GREY = "|cff808080";

/* Class Colors */
constexpr std::array<char const*, 11> world_chat_ClassColor =
{
    "|cffC79C6E", // WARRIOR
    "|cffF58CBA", // PALADIN
    "|cffABD473", // HUNTER
    "|cffFFF569", // ROGUE
    "|cffFFFFFF", // PRIEST
    "|cffC41F3B", // DEATHKNIGHT
    "|cff0070DE", // SHAMAN
    "|cff40C7EB", // MAGE
    "|cff8787ED", // WARLOCK
    "", // ADDED IN MOP FOR MONK - NOT USED
    "|cffFF7D0A", // DRUID
};

/* Ranks */
constexpr std::array<char const*, 4> world_chat_GM_RANKS =
{
    "Player",
    "MOD",
    "GM",
    "ADMIN",
};

/* BLIZZARD CHAT ICON FOR GM'S */
constexpr auto world_chat_GMIcon = "|TINTERFACE/CHATFRAME/UI-CHATICON-BLIZZ:13:13:0:-1|t";

/* COLORED TEXT FOR CURRENT FACTION || NOT FOR GMS */
constexpr std::array<char const*, 2> world_chat_TeamIcon =
{
    "|cff3399FFAlliance|r",
    "|cffCC0000Horde|r"
};

/* Config Variables */
struct WCConfig
{
    bool Enabled{};
    std::string ChannelName;
    bool LoginState{};
    bool CrossFaction{};
};

WCConfig WC_Config;

class WorldChat_Config : public WorldScript
{
public: WorldChat_Config() : WorldScript("WorldChat_Config") { }

    void OnBeforeConfigLoad(bool reload) override
    {
        if (!reload)
        {
            WC_Config.Enabled = sConfigMgr->GetOption<bool>("World_Chat.Enable", true);
            WC_Config.ChannelName = sConfigMgr->GetOption<std::string>("World_Chat.ChannelName", "World");
            WC_Config.LoginState = sConfigMgr->GetOption<bool>("World_Chat.OnLogin.State", true);
            WC_Config.CrossFaction = sConfigMgr->GetOption<bool>("World_Chat.CrossFactions", true);
        }
    }
};

/* STRUCTURE FOR WorldChat map */
struct ChatElements
{
    uint8 chat = (WC_Config.LoginState) ? 1 : 0; // CHAT DISABLED BY DEFAULT
};

/* UNORDERED MAP FOR STORING IF CHAT IS ENABLED OR DISABLED */
std::unordered_map<uint32, ChatElements> WorldChat;

void SendWorldMessage(Player* sender, std::string_view msg, int team)
{
    if (!WC_Config.Enabled)
    {
        ChatHandler(sender->GetSession()).PSendSysMessage("[WC] {}World Chat System is disabled.|r", WORLD_CHAT_RED);
        return;
    }

    if (!sender->GetSession()->CanSpeak())
    {
        ChatHandler(sender->GetSession()).PSendSysMessage("[WC] {}You can't use World Chat while muted!|r", WORLD_CHAT_RED);
        return;
    }

    if (!WorldChat[sender->GetGUID().GetCounter()].chat)
    {
        ChatHandler(sender->GetSession()).PSendSysMessage("[WC] {}World Chat is hidden. (.chat off)|r", WORLD_CHAT_RED);
        return;
    }

    for (auto& session : sWorld->GetAllSessions())
    {
        if (!session.second)
        {
            continue;
        }

        if (!session.second->GetPlayer())
        {
            continue;
        }

        if (!session.second->GetPlayer()->IsInWorld())
        {
            continue;
        }

        Player* target = session.second->GetPlayer();
        uint64 guid2 = target->GetGUID().GetCounter();

        if (WorldChat[guid2].chat == 1 && (team == -1 || target->GetTeamId() == team))
        {
            if (WC_Config.CrossFaction || (sender->GetTeamId() == target->GetTeamId()) || target->GetSession()->GetSecurity())
            {
                std::string message;

                if (sender->isGMChat())
                    message = Warhead::StringFormat("[World][{}][{}|Hplayer:{}|h{}|h|r]: {}{}|r", world_chat_GMIcon , world_chat_ClassColor[sender->getClass() - 1], sender->GetName(), sender->GetName().c_str(), WORLD_CHAT_WHITE, msg);
                else
                    message = Warhead::StringFormat("[World][{}][{}|Hplayer:{}|h{}|h|r]: {}{}|r", world_chat_TeamIcon[sender->GetTeamId()], world_chat_ClassColor[sender->getClass() - 1], sender->GetName().c_str(), sender->GetName().c_str(), WORLD_CHAT_WHITE, msg);

                ChatHandler(target->GetSession()).PSendSysMessage("{}", message);
            }
        }
    }
}

using namespace Warhead::ChatCommands;

class World_Chat : public CommandScript
{
public:
    World_Chat() : CommandScript("World_Chat") { }

    [[nodiscard]] ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable wcCommandTable =
        {
            { "on",      HandleWorldChatOnCommand,  SEC_PLAYER,  Console::No  },
            { "off",     HandleWorldChatOffCommand, SEC_PLAYER,  Console::No  },
            { "",        HandleWorldChatCommand,    SEC_PLAYER,  Console::No  }
        };

        static ChatCommandTable commandTable =
        {
            { "chat", wcCommandTable },
            { "chath", HandleWorldChatAllianceCommand, SEC_PLAYER,  Console::No  },
            { "chata", HandleWorldChatHordeCommand, SEC_PLAYER,  Console::No  },
        };

        return commandTable;
    }

    static bool HandleWorldChatCommand(ChatHandler* pChat, std::string_view msg)
    {
        if (msg.empty())
            return false;

        SendWorldMessage(pChat->GetSession()->GetPlayer(), msg, -1);
        return true;
    }

    static bool HandleWorldChatHordeCommand(ChatHandler* pChat, std::string_view msg)
    {
        if (msg.empty())
            return false;

        SendWorldMessage(pChat->GetSession()->GetPlayer(), msg, TEAM_HORDE);
        return true;
    }

    static bool HandleWorldChatAllianceCommand(ChatHandler* pChat, std::string_view msg)
    {
        if (msg.empty())
            return false;

        SendWorldMessage(pChat->GetSession()->GetPlayer(), msg, TEAM_ALLIANCE);
        return true;
    }

    static bool HandleWorldChatOnCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        uint64 guid = player->GetGUID().GetCounter();

        if (!WC_Config.Enabled)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("[WC] {}World Chat System is disabled.|r", WORLD_CHAT_RED);
            return true;
        }

        if (WorldChat[guid].chat)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("[WC] {}World Chat is already visible.|r", WORLD_CHAT_RED);
            return true;
        }

        WorldChat[guid].chat = 1;
        ChatHandler(player->GetSession()).PSendSysMessage("[WC] {}World Chat is now visible.|r", WORLD_CHAT_GREEN);
        return true;
    };

    static bool HandleWorldChatOffCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        uint64 guid = player->GetGUID().GetCounter();

        if (!sConfigMgr->GetOption<bool>("World_Chat.Enable", true))
        {
            ChatHandler(player->GetSession()).PSendSysMessage("[WC] {}World Chat System is disabled.|r", WORLD_CHAT_RED);
            return true;
        }

        if (!WorldChat[guid].chat)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("[WC] {}World Chat is already hidden.|r", WORLD_CHAT_RED);
            return true;
        }

        WorldChat[guid].chat = 0;
        ChatHandler(player->GetSession()).PSendSysMessage("[WC] {}World Chat is now hidden.|r", WORLD_CHAT_GREEN);
        return true;
    };
};

class WorldChat_Announce : public PlayerScript
{
public:
    WorldChat_Announce() : PlayerScript("WorldChat_Announce") {}

    void OnChat(Player* player, uint32 /*type*/, uint32 lang, std::string& msg, Channel* channel) override
    {
        if (!WC_Config.ChannelName.empty() && lang != LANG_ADDON && !strcmp(channel->GetName().c_str(), WC_Config.ChannelName.c_str()))
        {
            SendWorldMessage(player, msg, -1);
            msg.clear();
        }
    }
};

void AddSC_WorldChatScripts()
{
    new WorldChat_Announce();
    new WorldChat_Config();
    new World_Chat();
}