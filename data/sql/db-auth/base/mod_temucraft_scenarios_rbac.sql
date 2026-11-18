-- mod-temucraft-scenarios: its game master commands as RBAC permissions (101101-101101). Each is linked to the
-- stock security role of its old level (194 moderator, 193 game master, 192 administrator) and to the matching
-- Temucraft GM tier (1011 Triager, 1012 Entertainer, 1014 Administrator). Grant or deny them per account with
-- .rbac account grant / deny, or link them to other roles.
DELETE FROM `rbac_linked_permissions` WHERE `linkedId` BETWEEN 101101 AND 101199;
DELETE FROM `rbac_permissions` WHERE `id` BETWEEN 101101 AND 101199;
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
(101101, 'Command: scenario unga');
INSERT INTO `rbac_linked_permissions` (`id`, `linkedId`) VALUES
(193, 101101),
(1012, 101101);
