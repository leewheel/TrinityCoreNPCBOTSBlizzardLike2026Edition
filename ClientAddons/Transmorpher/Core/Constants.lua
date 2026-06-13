local addon, ns = ...

-- ============================================================
-- TRANSMORPHER CONSTANTS
-- Centralized color palette, dimensions, slot data, race data
-- ============================================================

ns.VERSION = "2.0.0"
ns.ADDON_PREFIX = "Transmorpher"

-- ============================================================
-- COLOR PALETTE
-- ============================================================
ns.Colors = {
    -- Primary gold accents
    gold        = { 1.00, 0.82, 0.20 },
    goldDark    = { 0.80, 0.65, 0.22 },
    goldLight   = { 0.95, 0.88, 0.65 },
    goldMuted   = { 0.60, 0.50, 0.18 },
    goldText    = { 0.95, 0.93, 0.88 },
    goldOrange  = { 0.96, 0.78, 0.26 },

    -- Backgrounds
    bgDark      = { 0.04, 0.03, 0.03 },
    bgMedium    = { 0.08, 0.06, 0.03 },
    bgLight     = { 0.12, 0.10, 0.06 },
    bgPanel     = { 0.06, 0.05, 0.03 },

    -- Text
    textWhite   = { 1.00, 1.00, 1.00 },
    textGray    = { 0.70, 0.70, 0.70 },
    textMuted   = { 0.50, 0.50, 0.50 },
    textDesc    = { 0.60, 0.53, 0.40 },
    textLore    = { 0.53, 0.49, 0.42 },

    -- Glow colors (for morphed slots)
    glowGold    = { 1.00, 0.85, 0.20, 0.60 },
    glowGreen   = { 0.20, 1.00, 0.20, 0.50 },
    glowOrange  = { 1.00, 0.50, 0.00, 0.55 },
    glowRed     = { 1.00, 0.20, 0.20, 0.55 },
    glowPurple  = { 0.70, 0.30, 1.00, 0.55 },

    -- Status
    statusGreen = { 0.29, 0.80, 0.29 },
    statusRed   = { 0.80, 0.29, 0.29 },
}

-- ============================================================
-- DIMENSIONS
-- ============================================================
ns.Dimensions = {
    mainWidth   = 1045,
    mainHeight  = 528,
    tabHeight   = 30,
    slotSize    = 40,
    slotGap     = 3,
    dressingW   = 350,
    dressingH   = 430,
}

-- ============================================================
-- SLOT DEFINITIONS
-- ============================================================

ns.armorSlots = {"Head", "Shoulder", "Chest", "Wrist", "Hands", "Waist", "Legs", "Feet"}
ns.backSlot = "Back"
ns.miscSlots = {"Tabard", "Shirt"}
ns.mainHandSlot = "Main Hand"
ns.offHandSlot = "Off-hand"
ns.rangedSlot = "Ranged"
ns.chestSlots = {"Chest", "Tabard", "Shirt"}

ns.slotOrder = {
    "Head", "Shoulder", "Back", "Chest", "Shirt", "Tabard",
    "Wrist", "Hands", "Waist", "Legs", "Feet",
    "Main Hand", "Off-hand", "Ranged",
}

ns.enchantSlotNames = { "Enchant MH", "Enchant OH" }

-- WoW equipment slot IDs for DLL morph calls
ns.slotToEquipSlotId = {
    ["Head"] = 1, ["Shoulder"] = 3, ["Back"] = 15, ["Chest"] = 5,
    ["Shirt"] = 4, ["Tabard"] = 19, ["Wrist"] = 9, ["Hands"] = 10,
    ["Waist"] = 6, ["Legs"] = 7, ["Feet"] = 8,
    ["Main Hand"] = 16, ["Off-hand"] = 17, ["Ranged"] = 18,
}

-- Reverse mapping
ns.equipSlotIdToSlot = {}
for name, id in pairs(ns.slotToEquipSlotId) do
    ns.equipSlotIdToSlot[id] = name
end

-- Slot background textures
ns.slotTextures = {
    ["Head"]       = "Interface\\Paperdoll\\ui-paperdoll-slot-head",
    ["Shoulder"]   = "Interface\\Paperdoll\\ui-paperdoll-slot-shoulder",
    ["Back"]       = "Interface\\Paperdoll\\ui-paperdoll-slot-chest",
    ["Chest"]      = "Interface\\Paperdoll\\ui-paperdoll-slot-chest",
    ["Shirt"]      = "Interface\\Paperdoll\\ui-paperdoll-slot-shirt",
    ["Tabard"]     = "Interface\\Paperdoll\\ui-paperdoll-slot-tabard",
    ["Wrist"]      = "Interface\\Paperdoll\\ui-paperdoll-slot-wrists",
    ["Hands"]      = "Interface\\Paperdoll\\ui-paperdoll-slot-hands",
    ["Waist"]      = "Interface\\Paperdoll\\ui-paperdoll-slot-waist",
    ["Legs"]       = "Interface\\Paperdoll\\ui-paperdoll-slot-legs",
    ["Feet"]       = "Interface\\Paperdoll\\ui-paperdoll-slot-feet",
    ["Main Hand"]  = "Interface\\Paperdoll\\ui-paperdoll-slot-mainhand",
    ["Off-hand"]   = "Interface\\Paperdoll\\ui-paperdoll-slot-secondaryhand",
    ["Ranged"]     = "Interface\\Paperdoll\\ui-paperdoll-slot-ranged",
}

-- ============================================================
-- SUBCLASS DATA (weapon/armor types per slot)
-- ============================================================
ns.slotSubclasses = {}

do
    for _, slot in ipairs(ns.armorSlots) do
        ns.slotSubclasses[slot] = {"Cloth", "Leather", "Mail", "Plate"}
    end
    for _, slot in ipairs(ns.miscSlots) do
        ns.slotSubclasses[slot] = {"Miscellaneous"}
    end
    ns.slotSubclasses[ns.backSlot] = {"Cloth"}

    local allWeaponTypes = {
        "1H Axe", "1H Mace", "1H Sword", "1H Dagger", "1H Fist",
        "MH Axe", "MH Mace", "MH Sword", "MH Dagger", "MH Fist",
        "OH Axe", "OH Mace", "OH Sword", "OH Dagger", "OH Fist",
        "2H Axe", "2H Mace", "2H Sword", "Polearm", "Staff",
        "Shield", "Held in Off-hand",
        "Bow", "Crossbow", "Gun", "Wand", "Thrown",
    }
    ns.slotSubclasses[ns.mainHandSlot] = allWeaponTypes
    ns.slotSubclasses[ns.offHandSlot]  = allWeaponTypes
    ns.slotSubclasses[ns.rangedSlot]   = allWeaponTypes
end

-- Default armor subclass per class
ns.defaultArmorSubclass = {
    ["MAGE"] = "Cloth", ["PRIEST"] = "Cloth", ["WARLOCK"] = "Cloth",
    ["DRUID"] = "Leather", ["ROGUE"] = "Leather",
    ["HUNTER"] = "Mail", ["SHAMAN"] = "Mail",
    ["PALADIN"] = "Plate", ["WARRIOR"] = "Plate", ["DEATHKNIGHT"] = "Plate",
}

-- ============================================================
-- RACE MORPH DISPLAY IDs
-- ============================================================
ns.raceDisplayIds = {
    ["人类"]      = { [2] = 19723, [3] = 19724 },
    ["兽人"]        = { [2] = 6785,  [3] = 20316 },
    ["矮人"]      = { [2] = 20317, [3] = 13250 },
    ["暗夜精灵"]  = { [2] = 20318, [3] = 2222  },
    ["亡灵"]     = { [2] = 28193, [3] = 23112 },
    ["牛头人"]     = { [2] = 20585, [3] = 20584 },
    ["侏儒"]      = { [2] = 20580, [3] = 20581 },
    ["巨魔"]      = { [2] = 20321, [3] = 4358  },
    ["血精灵"]  = { [2] = 20578, [3] = 20579 },
    ["德莱尼"]    = { [2] = 17155, [3] = 20323 },
}
ns.raceOrder = {"人类", "兽人", "矮人", "暗夜精灵", "亡灵", "牛头人", "侏儒", "巨魔", "血精灵", "德莱尼"}

-- ============================================================
-- POPULAR CREATURES (for Morph tab quick buttons)
-- ============================================================
ns.popularCreatures = {
    { name = "巫妖王",     id = 22234 },
    { name = "伊利丹",       id = 21135 },
    { name = "希尔瓦娜斯",      id = 28213 },
    { name = "阿莱克丝塔萨",   id = 28227 },
    { name = "拉格纳罗斯",      id = 11121 },
    { name = "布莱恩·铜须", id = 22266 },
    { name = "玛里苟斯",       id = 26752 },
    { name = "海象人",       id = 24685 },
    { name = "克尔苏加德",    id = 15945 },
    { name = "尤格-萨隆",    id = 28817 },
    { name = "凯尔萨斯",     id = 20023 },
    { name = "瓦斯琪女士",    id = 20748 },
    { name = "奈法利安",      id = 11380 },
    { name = "奥妮克希亚",        id = 8570  },
    { name = "阿尔萨斯",        id = 24949 },
    { name = "乌瑟尔",         id = 16929 },
    { name = "邪恶阿尔萨斯",   id = 22235 },
    { name = "维伦",         id = 23749 },
    { name = "黑暗瓦格里",  id = 25517 },
    { name = "企鹅",       id = 24698 },
}

-- ============================================================
-- COMBAT PET FAMILY ICONS
-- ============================================================
ns.combatPetFamilyIcons = {
    ["Bear"]          = "Interface\\Icons\\Ability_Hunter_Pet_Bear",
    ["Boar"]          = "Interface\\Icons\\Ability_Hunter_Pet_Boar",
    ["Cat"]           = "Interface\\Icons\\Ability_Hunter_Pet_Cat",
    ["Carrion Bird"]  = "Interface\\Icons\\Ability_Hunter_Pet_Vulture",
    ["Crab"]          = "Interface\\Icons\\Ability_Hunter_Pet_Crab",
    ["Crocolisk"]     = "Interface\\Icons\\Ability_Hunter_Pet_Crocolisk",
    ["Dragonhawk"]    = "Interface\\Icons\\Ability_Hunter_Pet_Dragonhawk",
    ["Gorilla"]       = "Interface\\Icons\\Ability_Hunter_Pet_Gorilla",
    ["Hyena"]         = "Interface\\Icons\\Ability_Hunter_Pet_Hyena",
    ["Moth"]          = "Interface\\Icons\\Ability_Hunter_Pet_Moth",
    ["Nether Ray"]    = "Interface\\Icons\\Ability_Hunter_Pet_NetherRay",
    ["Raptor"]        = "Interface\\Icons\\Ability_Hunter_Pet_Raptor",
    ["Ravager"]       = "Interface\\Icons\\Ability_Hunter_Pet_Ravager",
    ["Scorpid"]       = "Interface\\Icons\\Ability_Hunter_Pet_Scorpid",
    ["Serpent"]        = "Interface\\Icons\\Ability_Hunter_Pet_WindSerpent",
    ["Spider"]        = "Interface\\Icons\\Ability_Hunter_Pet_Spider",
    ["Sporebat"]      = "Interface\\Icons\\Ability_Hunter_Pet_Sporebat",
    ["Tallstrider"]   = "Interface\\Icons\\Ability_Hunter_Pet_Tallstrider",
    ["Turtle"]        = "Interface\\Icons\\Ability_Hunter_Pet_Turtle",
    ["Warp Stalker"]  = "Interface\\Icons\\Ability_Hunter_Pet_WarpStalker",
    ["Wasp"]          = "Interface\\Icons\\Ability_Hunter_Pet_Wasp",
    ["Wolf"]          = "Interface\\Icons\\Ability_Hunter_Pet_Wolf",
    ["Worm"]          = "Interface\\Icons\\Ability_Hunter_Pet_Worm",
    ["Bat"]           = "Interface\\Icons\\Ability_Hunter_Pet_Bat",
    ["Chimaera"]      = "Interface\\Icons\\Ability_Hunter_Pet_Chimera",
    ["Core Hound"]    = "Interface\\Icons\\Ability_Hunter_Pet_CoreHound",
    ["Devilsaur"]     = "Interface\\Icons\\Ability_Hunter_Pet_Devilsaur",
    ["Rhino"]         = "Interface\\Icons\\Ability_Hunter_Pet_Rhino",
    ["Silithid"]      = "Interface\\Icons\\Ability_Hunter_Pet_Silithid",
    ["Demon"]         = "Interface\\Icons\\Spell_Shadow_SummonFelHunter",
    ["Elemental"]     = "Interface\\Icons\\Spell_Frost_SummonWaterElemental_2",
}

-- Combat pet family Chinese names (for filter display)
ns.combatPetFamilyCN = {
    ["Bear"] = "熊", ["Boar"] = "野猪", ["Cat"] = "猫", ["Carrion Bird"] = "食腐鸟",
    ["Crab"] = "蟹", ["Crocolisk"] = "鳄鱼", ["Dragonhawk"] = "龙鹰", ["Gorilla"] = "猩猩",
    ["Hyena"] = "土狼", ["Moth"] = "飞蛾", ["Nether Ray"] = "虚空鳐", ["Raptor"] = "迅猛龙",
    ["Ravager"] = "掠食者", ["Scorpid"] = "蝎子", ["Serpent"] = "毒蛇", ["Spider"] = "蜘蛛",
    ["Sporebat"] = "孢子蝙蝠", ["Tallstrider"] = "陆行鸟", ["Turtle"] = "海龟",
    ["Warp Stalker"] = "迁跃捕猎者", ["Wasp"] = "黄蜂", ["Wolf"] = "狼",
    ["Worm"] = "蠕虫", ["Bat"] = "蝙蝠", ["Chimaera"] = "奇美拉",
    ["Core Hound"] = "熔核犬", ["Devilsaur"] = "魔暴龙", ["Rhino"] = "犀牛",
    ["Silithid"] = "其拉虫", ["Demon"] = "恶魔", ["Elemental"] = "元素",
    ["Creature"] = "生物",
}

-- ============================================================
-- DEFAULT SETTINGS
-- ============================================================
ns.defaultSettings = {
    dressingRoomBackgroundColor = {0.6, 0.6, 0.6, 1},
    previewSetup = "classic",
    showDressMeButton = true,
    useServerTimeInReceivedAppearances = false,
    ignoreUIScaling = false,
    saveMorphState = true,
    saveMountMorph = true,
    savePetMorph = true,
    saveHunterPetMorph = true,
    saveCombatPetMorph = true,
    showMetamorphosis = true,
    showDBWProc = false,
    morphInShapeshift = false,
    worldTime = nil,
    enableWorldSync = true,
    worldFogEnabled = false,
    worldFogColor = "#AAAAAA",
    worldFogStart = 500,
    worldFogEnd = 2500,
    worldFarClipEnabled = false,
    worldFarClip = 2666,
    worldRenderLiquidSurface = true,
    worldRenderLiquidParticles = true,
    worldRenderWireframe = false,
    worldRenderNormals = false,
    worldRenderTerrain = true,
    worldRenderTerrainCulling = true,
    worldRenderM2 = true,
    worldRenderM2WmoShadow = true,
    worldRenderWmo = true,
    worldRenderWmoLighting = true,
    worldRenderFootprints = true,
    worldRenderWmoTextures = true,
    worldRenderWmoPortals = false,
    worldRenderOccluders = false,
    worldRenderM2Fade = true,
    worldRenderGroundClutter = true,
    worldRenderCollision = false,
    worldRenderMountains = true,
    worldRenderSpecularLighting = true,
    worldRenderObjectShadow = true,
    worldRenderSmoothTextures = false,
    worldRenderSmoothTexturesBias = 1.25,
    miscHdFontMode = 0,
    talentLoadoutBindings = {},
    maxVisiblePlayers = 0,
    showMinimapButton = true,
    hidePaperdollButton = false,
    -- Optimization (Spell Visibility)
    hideAllSpells            = false,
    showOwnSpells            = false,
    hidePrecast              = false,
    hideCast                 = false,
    hideChannel              = false,
    hideAuraStart            = false,
    hideAuraEnd              = false,
    hideImpact               = false,
    hideImpactCaster         = false,
    hideTargetImpact         = false,
    hideAreaInstant          = false,
    hideAreaImpact           = false,
    hideAreaPersistent       = false,
    hideMissile              = false,
    hideMissileMarker        = false,
    hideSoundMissile         = false,
    hideSoundEvent           = false,
    protectTierT10           = false,
    protectTierT9            = false,
    protectTierT8            = false,
    protectTierT7            = false,
    protectTierVOA           = false,
    whiteCardSpells          = {},
}

ns.optimizationTierOptions = {
    { key = "T10", settingKey = "protectTierT10", label = "T10", raids = "ICC + RS" },
    { key = "T9",  settingKey = "protectTierT9",  label = "T9",  raids = "TOC + Onyxia" },
    { key = "T8",  settingKey = "protectTierT8",  label = "T8",  raids = "Ulduar" },
    { key = "T7",  settingKey = "protectTierT7",  label = "T7",  raids = "OS + Naxx + EoE" },
    { key = "VOA", settingKey = "protectTierVOA", label = "VOA", raids = "Vault of Archavon" },
}

-- ============================================================
-- DEATHBRINGER'S WILL PROC IDs
-- ============================================================
ns.dbwProcIds = {
    [71484] = true, [71561] = true,
    [71486] = true, [71558] = true,
    [71485] = true, [71556] = true,
    [71492] = true, [71560] = true,
    [71491] = true, [71559] = true,
    [71487] = true, [71557] = true,
}

-- ============================================================
-- TAB CONFIGURATION
-- ============================================================
ns.tabConfig = {
    { key = "preview",    label = "Preview",     icon = "Interface\\Icons\\INV_Chest_Cloth_17" },
    { key = "appearances",label = "Loadouts",    icon = "Interface\\Icons\\INV_Misc_Book_11" },
    { key = "mounts",     label = "Mounts",      icon = "Interface\\Icons\\Ability_Mount_RidingHorse" },
    { key = "pets",       label = "Pets",         icon = "Interface\\Icons\\INV_Box_PetCarrier_01" },
    { key = "combatPets", label = "CPets",        icon = "Interface\\Icons\\Ability_Hunter_BeastCall" },
    { key = "morph",      label = "Morph",        icon = "Interface\\Icons\\Spell_Shadow_Charm" },
    { key = "color",      label = "Color",        icon = "Interface\\Icons\\INV_Misc_Gem_Pearl_06" },
    { key = "env",        label = "Misc",         icon = "Interface\\Icons\\Spell_Nature_EarthBind" },
    { key = "settings",   label = "Settings",     icon = "Interface\\Icons\\INV_Misc_Gear_01" },
}

-- ============================================================
-- BACKDROP TEMPLATES
-- ============================================================
ns.Backdrops = {
    panel = {
        bgFile = "Interface\\ChatFrame\\ChatFrameBackground",
        edgeFile = "Interface\\Tooltips\\UI-Tooltip-Border",
        tile = true, tileSize = 16, edgeSize = 16,
        insets = { left = 3, right = 3, top = 3, bottom = 3 },
    },
    panelSmall = {
        bgFile = "Interface\\ChatFrame\\ChatFrameBackground",
        edgeFile = "Interface\\Tooltips\\UI-Tooltip-Border",
        tile = true, tileSize = 16, edgeSize = 12,
        insets = { left = 3, right = 3, top = 3, bottom = 3 },
    },
    button = {
        edgeFile = "Interface\\Tooltips\\UI-Tooltip-Border",
        edgeSize = 12,
        insets = { left = 2, right = 2, top = 2, bottom = 2 },
    },
    dressingRoom = {
        bgFile = "Interface\\ChatFrame\\ChatFrameBackground",
        edgeFile = "Interface\\AddOns\\Transmorpher\\images\\mirror-border",
        tile = false, tileSize = 16, edgeSize = 32,
        insets = { left = 4, right = 4, top = 4, bottom = 4 },
    },
}

-- ============================================================
-- VEHICLE DETECTION KEYWORDS
-- ============================================================
ns.vehicleKeywords = {
    "Chopper", "Salvaged", "Demolisher", "Siege", "Engine", "Cannon", "Canon", "Harpoon",
    "Turret", "Teleporter", "Drake", "Dragon", "Tank", "Golem", "Robot", "Machine",
    "Plane", "Ship", "Boat", "Zeppelin", "Bomber", "Steam", "Flame",
    "Leviathan", "Mimiron", "Gryphon", "Wyvern", "Bat", "Hawkstrider", "Catapult",
    "Car", "Shuttle", "Submarine", "Valkyrie", "Mammoth", "Motor", "Bike", "Cycle",
    "Rider", "Pilot", "Gunner", "Azure", "Amber", "Emerald", "Scion", "Proto-Drake",
    "Aerial", "Command", "Platform", "Guardian", "Sentinel", "Constructor",
    "Mechano", "Turbo", "Automatic", "Flying", "Hover", "Glider", "Sled", "Rocket",
    "Blimp", "Balloon", "Gnome", "Goblin", "Experimental", "Security",
    "Defense", "Assault", "War", "Combat", "Battle", "Transport", "Portal",
    "Focus", "Nexus", "Pulse", "Energy", "Beam", "Static", "Launcher", "Ram",
    "Stabled Thunder Bluff Kodo", "Stabled Darkspear Raptor", "Stabled Forsaken Warhorse",
    "Stabled Orgrimmar Wolf", "Stabled Silvermoon Hawkstrider", "Stabled Sunreaver Hawkstrider",
    "Stabled Argent Warhorse",
}
