# mod-temucraft-scenarios

Mists of Pandaria scenarios as short scripted instances for 1-3 players, all in C++.

## Unga Ingoo (map 1048)

The three stages of the 5.4.8 scenario:

1. **Escort Brewmaster Bo**: talk to Bo at the landing; he walks to the Unga Ingoo brewing cauldron.
2. **Fill the Brewing Cauldron**: 200 Unga Jungle Brew: every slain hozen or pirate adds some, and the mugs, kegs
   and jungle brew around the island can be picked up for more.
3. **Defeat Captain Ook**: Ook can't be attacked until the cauldron is full.

The 3.3.5 client has no scenario objectives frame, so each stage and the brew count are shown in the middle of the
screen and in chat. On completion every player gets the configured reward.

Battle on the High Seas (map 1099) isn't in yet: no database the scripts came from has its spawns.

## Getting in

- **Scenario Guide** (creature 608803, Lorewalker Pang): place it where you like (`.npc add 608803`) and pick a
  scenario from its menu. Level and group-size limits are in the config.
- GMs: `.temucraft scenario unga`.

## Install

1. Install [mod-temucraft-world](https://github.com/Temucraft/mod-temucraft-world) first: it holds the scenario's spawns and turns the scenario maps into dungeons
   (`map_dbc`, `mapdifficulty_dbc`).
2. Put this folder in `modules/`, copy `conf/mod-temucraft-scenarios.conf.dist` to your config folder as
   `mod-temucraft-scenarios.conf`, re-run CMake and build.
3. The SQL in `data/sql/` is applied by the worldserver on its next start.

## Settings

`Temucraft.Scenarios.Enable`, `MinLevel`, `MaxPlayers`, `RewardGold`, `RewardItem`, `RewardItemCount`, and for
Unga Ingoo `BrewTarget`, `BrewPerKill`, `BrewPerObject`. See the `.conf.dist` file.

## What it changes

| Database | Table | What |
|---|---|---|
| world | `creature_template`, `creature_template_model`, `npc_text` | the Scenario Guide (608803) |
| world | `instance_template` | Unga Ingoo's instance script |
| world | `creature_template`, `gameobject_template` | `ScriptName` of Brewmaster Bo and the brew objects |
| world | `module_string` | the module's texts |

## Texts and translations

The player-facing texts are in the `module_string` table under the module name `mod-temucraft-scenarios`.
Translations go in `module_string_locale` with the same module and id plus the locale. If a row is missing the
English text in the code is used.

## Works with other modules

- Needs [mod-temucraft-world](https://github.com/Temucraft/mod-temucraft-world). The planned expedition module can send parties into the same scenarios.

Part of the Temucraft modules for AzerothCore 3.3.5a.
