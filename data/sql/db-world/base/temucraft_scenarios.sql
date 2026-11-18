-- mod-temucraft-scenarios: the Scenario Guide, the scenario's script names and the module's texts
DELETE FROM `creature_template` WHERE `entry` = 608803;
INSERT INTO `creature_template` (`entry`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction`, `npcflag`,
  `speed_walk`, `speed_run`, `rank`, `unit_class`, `unit_flags`, `type`, `type_flags`, `AIName`, `MovementType`, `HealthModifier`, `RegenHealth`,
  `flags_extra`, `ScriptName`) VALUES
(608803, 'Lorewalker Pang', 'Scenario Guide', '', 0, 80, 80, 2, 35, 1, 1, 1.14286, 0, 1, 768, 7, 0, '', 0, 1, 1, 2, 'npc_temucraft_scenario_guide');
DELETE FROM `creature_template_model` WHERE `CreatureID` = 608803;
INSERT INTO `creature_template_model` (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`, `VerifiedBuild`)
SELECT 608803, 0, `CreatureDisplayID`, 1, 1, 0 FROM `creature_template_model` WHERE `CreatureID` = 28693 AND `Idx` = 0;

DELETE FROM `npc_text` WHERE `ID` = 608803;
INSERT INTO `npc_text` (`ID`, `text0_0`, `text0_1`, `Probability0`) VALUES
(608803, 'Every story is better lived than told. Pick one, gather a friend or two, and I will send you right into it.', '', 1);

-- Unga Ingoo (map 1048): the instance script, Brewmaster Bo and the brew you can pick up
INSERT INTO `instance_template` (`map`, `parent`, `script`, `allowMount`) VALUES (1048, 0, 'instance_temucraft_unga_ingoo', 0)
  ON DUPLICATE KEY UPDATE `script` = VALUES(`script`);
UPDATE `creature_template` SET `ScriptName` = 'npc_temucraft_unga_bo', `npcflag` = `npcflag` | 1 WHERE `entry` = 62467;
UPDATE `gameobject_template` SET `ScriptName` = 'go_temucraft_unga_brew' WHERE `entry` IN (212248, 212253, 212254, 212255, 212256, 215853);

DELETE FROM `module_string` WHERE `module` = 'mod-temucraft-scenarios';
INSERT INTO `module_string` (`module`, `id`, `string`) VALUES
('mod-temucraft-scenarios', 1, 'Unga Ingoo (scenario, 1-3 players)'),
('mod-temucraft-scenarios', 2, 'You need to be level {} for scenarios.'),
('mod-temucraft-scenarios', 3, 'Scenarios are turned off.'),
('mod-temucraft-scenarios', 4, 'Escort Brewmaster Bo: bring him to the Unga Ingoo brewing cauldron.'),
('mod-temucraft-scenarios', 5, 'Fill the brewing cauldron: bring {1} Unga Jungle Brew. Slay hozen and grab their mugs and kegs.'),
('mod-temucraft-scenarios', 6, 'The cauldron is full! Captain Ook comes for his brew - defeat him!'),
('mod-temucraft-scenarios', 7, 'Unga Ingoo complete!'),
('mod-temucraft-scenarios', 8, 'Unga Jungle Brew: {}/{}'),
('mod-temucraft-scenarios', 9, 'Let''s get you to the cauldron, Bo.'),
('mod-temucraft-scenarios', 11, 'Right behind you, friend. Mind the hozen!'),
('mod-temucraft-scenarios', 13, 'Scenario reward: {} gold.'),
('mod-temucraft-scenarios', 15, 'Scenarios are for up to {} players.');
-- Translations go in `module_string_locale` (`module`, `id`, `locale`, `string`).
