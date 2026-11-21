/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/*
 * mod-temucraft-scenarios: Mists of Pandaria scenarios as short scripted instances for 1-3 players.
 *
 * Unga Ingoo (map 1048), in three stages as in 5.4.8:
 *   1. Escort Brewmaster Bo to the Unga Ingoo brewing cauldron.
 *   2. Fill the cauldron: 200 Unga Jungle Brew, from slain hozen and from the mugs and kegs around the island.
 *   3. Defeat Captain Ook.
 * The 3.3.5 client has no scenario objectives frame, so each stage and the brew count are announced on screen.
 *
 * Players get in through the Scenario Guide (creature 608803) or the GM command .temucraft scenario. The scenario maps are
 * made dungeons by mod-temucraft-world (map_dbc), which also holds their spawns.
 *
 * Text comes from module_string (module "mod-temucraft-scenarios"), with English built in as a fallback.
 */
#include "Chat.h"
#include "ChatCommand.h"
#include "Config.h"
#include "Creature.h"
#include "CreatureScript.h"
#include "GameObject.h"
#include "GameObjectScript.h"
#include "Group.h"
#include "InstanceMapScript.h"
#include "InstanceScript.h"
#include "Map.h"
#include "MotionMaster.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "StringFormat.h"
#include "TemucraftModuleText.h"
#include "WorldScript.h"
#include "CommandScript.h"

using namespace Acore::ChatCommands;

namespace
{
    constexpr char const* MODULE = "mod-temucraft-scenarios";
    constexpr uint32 MAP_UNGA_INGOO = 1048;

    enum UngaCreatures : uint32
    {
        NPC_BO_ESCORT = 62467,       // Brewmaster Bo at the landing
        NPC_BO_CAULDRON = 62491,     // Brewmaster Bo at the cauldron
        NPC_CAPTAIN_OOK = 62465,
    };
    // every hozen and pirate that counts towards the brew
    constexpr uint32 BREW_CREATURES[] = { 62494, 62492, 62495, 63256, 62590, 62474, 62478, 67028, 66195, 62473, 62508, 67027, 62475, 62466, 62468, 62485, 62486 };
    // mugs, kegs and jungle brew that can be picked up
    constexpr uint32 BREW_OBJECTS[] = { 212248, 212253, 212254, 212255, 212256, 215853 };

    enum Stage : uint32 { STAGE_ESCORT = 0, STAGE_BREW = 1, STAGE_OOK = 2, STAGE_DONE = 3 };
    enum Data : uint32 { DATA_STAGE = 1, DATA_BREW = 2 };

    enum Str : uint32
    {
        S_GUIDE_UNGA = 1, S_GUIDE_LEVEL, S_GUIDE_OFF, S_STAGE_ESCORT, S_STAGE_BREW, S_STAGE_OOK, S_DONE, S_BREW_COUNT,
        S_BO_GO, S_BO_GOSSIP_BUSY, S_BO_ARRIVED, S_LEAVE, S_REWARD_GOLD, S_GROUP_ONLY_LEADER, S_TOO_MANY
    };

    struct Settings
    {
        bool enable = true;
        uint32 minLevel = 80;
        uint32 maxPlayers = 3;
        uint32 brewTarget = 200;
        uint32 brewPerKill = 5;
        uint32 brewPerObject = 10;
        uint32 rewardGold = 500000;     // copper
        uint32 rewardItem = 0;
        uint32 rewardItemCount = 1;
        uint32 guideText = 608803;
    } cfg;

    void LoadSettings()
    {
        cfg.enable = sConfigMgr->GetOption<bool>("Temucraft.Scenarios.Enable", true);
        cfg.minLevel = sConfigMgr->GetOption<uint32>("Temucraft.Scenarios.MinLevel", 80);
        cfg.maxPlayers = sConfigMgr->GetOption<uint32>("Temucraft.Scenarios.MaxPlayers", 3);
        cfg.brewTarget = std::max<uint32>(1, sConfigMgr->GetOption<uint32>("Temucraft.Scenarios.UngaIngoo.BrewTarget", 200));
        cfg.brewPerKill = sConfigMgr->GetOption<uint32>("Temucraft.Scenarios.UngaIngoo.BrewPerKill", 5);
        cfg.brewPerObject = sConfigMgr->GetOption<uint32>("Temucraft.Scenarios.UngaIngoo.BrewPerObject", 10);
        cfg.rewardGold = sConfigMgr->GetOption<uint32>("Temucraft.Scenarios.RewardGold", 500000);
        cfg.rewardItem = sConfigMgr->GetOption<uint32>("Temucraft.Scenarios.RewardItem", 0);
        cfg.rewardItemCount = sConfigMgr->GetOption<uint32>("Temucraft.Scenarios.RewardItemCount", 1);
        cfg.guideText = sConfigMgr->GetOption<uint32>("Temucraft.Scenarios.GuideText", 608803);
    }

    template<typename T>
    char const* Text(T who, uint32 id, char const* fallback) { return TemucraftModuleText(MODULE, who, id, fallback); }

    // big centred text, as close to the scenario frame as 3.3.5 gets
    void Announce(Map* map, uint32 id, char const* fallback, uint32 value = 0, uint32 total = 0)
    {
        map->DoForAllPlayers([&](Player* player)
        {
            std::string text = Acore::StringFormat(Text(player, id, fallback), value, total);
            player->GetSession()->SendAreaTriggerMessage(std::string_view(text));
            ChatHandler(player->GetSession()).SendSysMessage(text);
        });
    }

    bool IsBrewCreature(uint32 entry)
    {
        for (uint32 e : BREW_CREATURES) if (e == entry) return true;
        return false;
    }
    bool IsBrewObject(uint32 entry)
    {
        for (uint32 e : BREW_OBJECTS) if (e == entry) return true;
        return false;
    }

    // ------------------------------------------------------------------------------------------- Unga Ingoo
    class instance_temucraft_unga_ingoo : public InstanceMapScript
    {
    public:
        instance_temucraft_unga_ingoo() : InstanceMapScript("instance_temucraft_unga_ingoo", MAP_UNGA_INGOO) { }

        struct instance_temucraft_unga_ingoo_InstanceMapScript : public InstanceScript
        {
            explicit instance_temucraft_unga_ingoo_InstanceMapScript(Map* map) : InstanceScript(map)
            {
                SetHeaders("TUI");
            }

            uint32 stage = STAGE_ESCORT;
            uint32 brew = 0;
            ObjectGuid boCauldron, ook;
            bool greeted = false;

            void OnCreatureCreate(Creature* creature) override
            {
                switch (creature->GetEntry())
                {
                    case NPC_BO_CAULDRON:
                        boCauldron = creature->GetGUID();
                        if (stage == STAGE_ESCORT)
                            creature->SetVisible(false);
                        break;
                    case NPC_CAPTAIN_OOK:
                        ook = creature->GetGUID();
                        if (stage < STAGE_OOK)
                        {
                            creature->SetUnitFlag(UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC);
                            creature->SetReactState(REACT_PASSIVE);
                        }
                        creature->SetFaction(14);
                        break;
                    case NPC_BO_ESCORT:
                        if (stage != STAGE_ESCORT)
                            creature->SetVisible(false);
                        break;
                }
            }

            void OnPlayerEnter(Player* player) override
            {
                InstanceScript::OnPlayerEnter(player);
                switch (stage)
                {
                    case STAGE_ESCORT: ChatHandler(player->GetSession()).SendSysMessage(Text(player, S_STAGE_ESCORT, "Escort Brewmaster Bo: bring him to the Unga Ingoo brewing cauldron.")); break;
                    case STAGE_BREW: ChatHandler(player->GetSession()).PSendSysMessage(Text(player, S_BREW_COUNT, "Unga Jungle Brew: {}/{}"), brew, cfg.brewTarget); break;
                    case STAGE_OOK: ChatHandler(player->GetSession()).SendSysMessage(Text(player, S_STAGE_OOK, "Defeat Captain Ook!")); break;
                    default: break;
                }
            }

            void AddBrew(uint32 amount)
            {
                if (stage != STAGE_BREW || !amount)
                    return;
                uint32 before = brew;
                brew = std::min(cfg.brewTarget, brew + amount);
                if (brew >= cfg.brewTarget)
                {
                    SetStage(STAGE_OOK);
                    return;
                }
                // a line every 10% of the way, plus the first brew
                uint32 step = std::max<uint32>(1, cfg.brewTarget / 10);
                if (before == 0 || before / step != brew / step)
                    Announce(instance, S_BREW_COUNT, "Unga Jungle Brew: {}/{}", brew, cfg.brewTarget);
                SaveToDB();
            }

            void SetStage(uint32 newStage)
            {
                stage = newStage;
                switch (stage)
                {
                    case STAGE_BREW:
                        if (Creature* bo = instance->GetCreature(boCauldron))
                            bo->SetVisible(true);
                        Announce(instance, S_STAGE_BREW, "Fill the brewing cauldron: bring {1} Unga Jungle Brew. Slay hozen and grab their mugs and kegs.", 0, cfg.brewTarget);
                        break;
                    case STAGE_OOK:
                        if (Creature* captain = instance->GetCreature(ook))
                        {
                            captain->RemoveUnitFlag(UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC);
                            captain->SetReactState(REACT_AGGRESSIVE);
                        }
                        Announce(instance, S_STAGE_OOK, "The cauldron is full! Captain Ook comes for his brew - defeat him!");
                        break;
                    case STAGE_DONE:
                        Announce(instance, S_DONE, "Unga Ingoo complete!");
                        instance->DoForAllPlayers([](Player* player)
                        {
                            if (cfg.rewardGold)
                            {
                                player->ModifyMoney(cfg.rewardGold);
                                ChatHandler(player->GetSession()).PSendSysMessage(Text(player, S_REWARD_GOLD, "Scenario reward: {} gold."), cfg.rewardGold / 10000);
                            }
                            if (cfg.rewardItem)
                                player->AddItem(cfg.rewardItem, cfg.rewardItemCount);
                        });
                        break;
                }
                SaveToDB();
            }

            void OnUnitDeath(Unit* unit) override
            {
                Creature* creature = unit->ToCreature();
                if (!creature)
                    return;
                if (creature->GetEntry() == NPC_CAPTAIN_OOK && stage == STAGE_OOK)
                    SetStage(STAGE_DONE);
                else if (IsBrewCreature(creature->GetEntry()))
                    AddBrew(cfg.brewPerKill);
            }

            void SetData(uint32 type, uint32 data) override
            {
                if (type == DATA_STAGE && data > stage)
                    SetStage(data);
                else if (type == DATA_BREW)
                    AddBrew(data);
            }

            uint32 GetData(uint32 type) const override
            {
                return type == DATA_STAGE ? stage : type == DATA_BREW ? brew : 0;
            }

            void WriteSaveDataMore(std::ostringstream& data) override { data << stage << ' ' << brew; }
            void ReadSaveDataMore(std::istringstream& data) override { data >> stage >> brew; }
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const override
        {
            return new instance_temucraft_unga_ingoo_InstanceMapScript(map);
        }
    };

    // Brewmaster Bo at the landing: talk to him to start the walk to the cauldron
    class npc_temucraft_unga_bo : public CreatureScript
    {
    public:
        npc_temucraft_unga_bo() : CreatureScript("npc_temucraft_unga_bo") { }

        struct npc_temucraft_unga_boAI : public ScriptedAI
        {
            explicit npc_temucraft_unga_boAI(Creature* creature) : ScriptedAI(creature) { }
            bool walking = false;

            void MovementInform(uint32 type, uint32 id) override
            {
                if (type == POINT_MOTION_TYPE && id == 1)
                {
                    if (InstanceScript* instance = me->GetInstanceScript())
                        instance->SetData(DATA_STAGE, STAGE_BREW);
                    me->DespawnOrUnsummon(2s);
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const override { return new npc_temucraft_unga_boAI(creature); }

        bool OnGossipHello(Player* player, Creature* creature) override
        {
            ClearGossipMenuFor(player);
            InstanceScript* instance = creature->GetInstanceScript();
            if (instance && instance->GetData(DATA_STAGE) == STAGE_ESCORT)
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, Text(player, S_BO_GO, "Let's get you to the cauldron, Bo."), GOSSIP_SENDER_MAIN, 1);
            SendGossipMenuFor(player, player->GetGossipTextId(creature), creature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
        {
            CloseGossipMenuFor(player);
            if (action != 1)
                return true;
            auto* ai = CAST_AI(npc_temucraft_unga_boAI, creature->AI());
            if (!ai || ai->walking)
                return true;
            ai->walking = true;
            creature->SetNpcFlag(UNIT_NPC_FLAG_NONE);
            creature->SetWalk(true);
            creature->Say(Text(player, S_BO_ARRIVED, "Right behind you, friend. Mind the hozen!"), LANG_UNIVERSAL);
            // the cauldron Bo, where this one walks to
            Position cauldron(-2939.14f, 767.198f, 9.43278f, 3.24446f);
            creature->GetMotionMaster()->MovePoint(1, cauldron, FORCED_MOVEMENT_NONE, 0.f, true);
            return true;
        }
    };

    // mugs, kegs and jungle brew: clicking one adds brew and removes it
    class go_temucraft_unga_brew : public GameObjectScript
    {
    public:
        go_temucraft_unga_brew() : GameObjectScript("go_temucraft_unga_brew") { }

        bool OnGossipHello(Player* /*player*/, GameObject* go) override
        {
            if (InstanceScript* instance = go->GetInstanceScript())
                if (instance->GetData(DATA_STAGE) == STAGE_BREW)
                {
                    instance->SetData(DATA_BREW, cfg.brewPerObject);
                    go->SetLootState(GO_JUST_DEACTIVATED);
                    go->Delete();
                }
            return true;
        }
    };

    // ------------------------------------------------------------------------------------------- the way in
    bool SendIn(Player* player, uint32 mapId)
    {
        if (!cfg.enable)
        {
            ChatHandler(player->GetSession()).SendSysMessage(Text(player, S_GUIDE_OFF, "Scenarios are turned off."));
            return false;
        }
        if (player->GetLevel() < cfg.minLevel)
        {
            ChatHandler(player->GetSession()).PSendSysMessage(Text(player, S_GUIDE_LEVEL, "You need to be level {} for scenarios."), cfg.minLevel);
            return false;
        }
        if (Group* group = player->GetGroup())
        {
            if (group->GetMembersCount() > cfg.maxPlayers)
            {
                ChatHandler(player->GetSession()).PSendSysMessage(Text(player, S_TOO_MANY, "Scenarios are for up to {} players."), cfg.maxPlayers);
                return false;
            }
        }
        if (mapId == MAP_UNGA_INGOO)
            return player->TeleportTo(MAP_UNGA_INGOO, -3092.0f, 744.0f, 1.4f, 0.9f);
        return false;
    }

    class npc_temucraft_scenario_guide : public CreatureScript
    {
    public:
        npc_temucraft_scenario_guide() : CreatureScript("npc_temucraft_scenario_guide") { }

        bool OnGossipHello(Player* player, Creature* creature) override
        {
            ClearGossipMenuFor(player);
            AddGossipItemFor(player, GOSSIP_ICON_BATTLE, Text(player, S_GUIDE_UNGA, "Unga Ingoo (scenario, 1-3 players)"), GOSSIP_SENDER_MAIN, MAP_UNGA_INGOO);
            SendGossipMenuFor(player, cfg.guideText, creature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 action) override
        {
            CloseGossipMenuFor(player);
            SendIn(player, action);
            return true;
        }
    };

    class temucraft_scenarios_commands : public CommandScript
    {
    public:
        temucraft_scenarios_commands() : CommandScript("temucraft_scenarios_commands") { }

        ChatCommandTable GetCommands() const override
        {
            static ChatCommandTable sub =
            {
                { "unga", HandleUnga, 101101 /* SEC_GAMEMASTER -> RBAC */, Console::No },
            };
            static ChatCommandTable root = { { "temucraft scenario", sub } };
            return root;
        }

        static bool HandleUnga(ChatHandler* handler)
        {
            Player* player = handler->GetPlayer();
            return player && player->TeleportTo(MAP_UNGA_INGOO, -3092.0f, 744.0f, 1.4f, 0.9f);
        }
    };

    class temucraft_scenarios_world : public WorldScript
    {
    public:
        temucraft_scenarios_world() : WorldScript("temucraft_scenarios_world") { }
        void OnAfterConfigLoad(bool /*reload*/) override { LoadSettings(); }
    };
}

void AddTemucraftScenariosScripts()
{
    new temucraft_scenarios_world();
    new instance_temucraft_unga_ingoo();
    new npc_temucraft_unga_bo();
    new go_temucraft_unga_brew();
    new npc_temucraft_scenario_guide();
    new temucraft_scenarios_commands();
}
