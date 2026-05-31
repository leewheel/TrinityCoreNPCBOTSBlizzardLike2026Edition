#!/usr/bin/env python3
"""Generate SQL to expand hireable NPCBot templates for large wandering-bot populations."""

from __future__ import annotations

import random
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]  # .../sql
CUSTOM = ROOT / "custom" / "world"
TEMPLATE_SQL = CUSTOM / "npcbot_2000_00_00_00_creature_template.sql"
EQUIP_SQL = CUSTOM / "npcbot_2000_00_00_00_creature_equip_template.sql"
BASE_NPCBOTS = ROOT / "base" / "world_npcbots.sql"
OUT_SQL = ROOT / "updates" / "world" / "3.3.5" / "2026_05_30_00_world_npcbot_expand_templates.sql"

ENTRY_FIRST = 70600
ENTRY_LAST = 72799  # 2200 templates
BOTS_PER_CLASS = (ENTRY_LAST - ENTRY_FIRST + 1) // 10

WANDER_CLASSES = [
    (1, "Warrior", "warrior_bot"),
    (2, "Paladin", "paladin_bot"),
    (3, "Hunter", "hunter_bot"),
    (4, "Rogue", "rogue_bot"),
    (5, "Priest", "priest_bot"),
    (6, "Death Knight", "death_knight_bot"),
    (7, "Shaman", "shaman_bot"),
    (8, "Mage", "mage_bot"),
    (9, "Warlock", "warlock_bot"),
    (11, "Druid", "druid_bot"),
]

# creature_template.subname by class — synced from live world DB (70001-70599), not "xxx Bot".
SUBNAME_BY_CLASS: dict[int, str] = {
    1: "永恒戍卫",
    2: "白银之手",
    3: "荒野生存者",
    4: "军情七处",
    5: "圣光教会",
    6: "黑锋骑士团",
    7: "五行教",
    8: "黑暗料理师",
    9: "恶魔契约",
    11: "塞纳留斯学院",
}

SURNAMES = list(
    "赵钱孙李周吴郑王冯陈褚卫蒋沈韩杨朱秦尤许何吕施张孔曹严华金魏陶姜戚谢邹喻"
    "柏水窦章云苏潘葛奚范彭郎鲁韦昌马苗凤花方俞任袁柳酆鲍史唐费廉岑薛雷贺倪汤"
    "滕殷罗毕郝邬安常乐于时傅皮卞齐康伍余元卜顾孟平黄和穆萧尹姚邵湛汪祁毛禹狄"
    "米贝明臧计伏成戴谈宋茅庞熊纪舒屈项祝董梁杜阮蓝闵席季麻强贾路娄危江童颜郭"
    "梅盛林刁钟徐邱骆高夏蔡田樊胡凌霍虞万支柯昝管卢莫经房裘缪干解应宗丁宣贲邓"
    "郁单杭洪包诸左石崔吉钮龚程嵇邢滑裴陆荣翁荀羊於惠甄曲家封芮羿储靳汲邴糜松"
    "井段富巫乌焦巴弓牧隗山谷车侯宓蓬全郗班仰秋仲伊宫宁仇栾暴甘钭厉戎祖武符刘"
    "景詹束龙叶幸司韶郜黎蓟薄印宿白怀蒲邰从鄂索咸籍赖卓蔺屠蒙池乔阴鬱胥能苍双"
    "闻莘党翟谭贡劳逄姬申扶堵冉宰郦雍却璩桑桂濮牛寿通边扈燕冀郏浦尚农温别庄晏"
    "柴瞿阎充慕连茹习宦艾鱼容向古易慎戈廖庾终暨居衡步都耿满弘匡国文寇广禄阙东"
    "殴殳沃利蔚越夔隆师巩厍聂晁勾敖融冷訾辛阚那简饶空曾毋沙乜养鞠须丰巢关蒯相"
    "查后荆红游竺权逯盖益桓公万俟司马上官欧阳夏侯诸葛闻人东方赫连皇甫尉迟公羊"
)
GIVEN = list(
    "天地玄黄宇宙洪荒日月盈昃辰宿列张寒来暑往秋收冬藏闰余成岁律吕调阳云腾致雨露"
    "结为霜金生丽水玉出昆冈剑号巨阙珠称夜光果珍李柰菜重芥姜海咸河淡鳞潜羽翔龙"
    "师火帝鸟官人皇始制文字乃服衣裳推位让国有虞陶唐吊民伐罪周发殷汤坐朝问道垂"
    "拱平章爱育黎首臣伏戎羌遐迩一体率宾归王鸣凤在竹白驹食场化被草木赖及万方"
    "军文武德仁义礼智信勇谋略宏博精深清风明月星河剑影霜雪雷霆风云山海江河"
    "玄霄凌霄紫霄青霄玉霄碧霄洞霄绛霄苍霄霄汉飞羽流萤落霞孤鹜秋水长天"
)

random.seed(20260530)


def parse_creature_rows(path: Path) -> dict[int, str]:
    text = path.read_text(encoding="utf-8", errors="replace")
    rows: dict[int, str] = {}
    for match in re.finditer(r"^\((\d+),(.+)\),?\s*$", text, re.M):
        entry = int(match.group(1))
        rows[entry] = match.group(2)
    return rows


def parse_extras(path: Path) -> dict[int, tuple[int, int]]:
    extras: dict[int, tuple[int, int]] = {}
    for match in re.finditer(r"^\((\d+),(\d+),(\d+)\),?\s*$", path.read_text(encoding="utf-8", errors="replace"), re.M):
        entry = int(match.group(1))
        if entry < 70001 or entry >= ENTRY_FIRST:
            continue
        extras[entry] = (int(match.group(2)), int(match.group(3)))
    return extras


def parse_appearance(path: Path) -> dict[int, tuple]:
    appearance: dict[int, tuple] = {}
    for match in re.finditer(
        r"^\((\d+),'((?:\\'|[^'])*)',(\d+),(\d+),(\d+),(\d+),(\d+),(\d+)\),?\s*$",
        path.read_text(encoding="utf-8", errors="replace"),
        re.M,
    ):
        entry = int(match.group(1))
        if entry < 70001 or entry >= ENTRY_FIRST:
            continue
        appearance[entry] = tuple(int(match.group(i)) for i in range(3, 9))
    return appearance


def parse_equip(path: Path) -> dict[int, tuple[int, int, int]]:
    equip: dict[int, tuple[int, int, int]] = {}
    for match in re.finditer(r"^\((\d+),1,(\d+),(\d+),(\d+),-1\),?\s*$", path.read_text(encoding="utf-8", errors="replace"), re.M):
        entry = int(match.group(1))
        equip[entry] = (int(match.group(2)), int(match.group(3)), int(match.group(4)))
    return equip


def pick_prototypes(
    creature_rows: dict[int, str],
    extras: dict[int, tuple[int, int]],
    appearance: dict[int, tuple],
    equip: dict[int, tuple[int, int, int]],
) -> dict[int, list[int]]:
    by_class: dict[int, list[int]] = {cls: [] for cls, _, _ in WANDER_CLASSES}
    pet_scripts = ("_pet_bot", "mirror_image", "script_bot_giver")
    for entry, row in creature_rows.items():
        if entry < 70001 or entry >= ENTRY_FIRST:
            continue
        if entry == 70000:
            continue
        if any(p in row for p in pet_scripts):
            continue
        if entry not in extras:
            continue
        bot_class = extras[entry][0]
        if bot_class in by_class:
            by_class[bot_class].append(entry)
    for cls in by_class:
        if not by_class[cls]:
            raise RuntimeError(f"No prototype bots for class {cls}")
    return by_class


def make_name(used: set[str]) -> str:
    for _ in range(10000):
        if random.random() < 0.35:
            name = random.choice(SURNAMES) + random.choice(GIVEN) + random.choice(GIVEN)
        else:
            name = random.choice(SURNAMES) + random.choice(GIVEN)
        if len(name) >= 2 and name not in used:
            used.add(name)
            return name
    raise RuntimeError("Ran out of unique names")


def clone_creature_row(proto_tail: str, entry: int, name: str, subname: str, script: str) -> str:
    tail = proto_tail.rstrip()
    if tail.endswith(")"):
        tail = tail[:-1]
    parts = tail.split(",", 2)
    rest = parts[2]
    rest = re.sub(r"'[^']*','[^']*'", f"'{name}','{subname}'", rest, count=1)
    rest = re.sub(r",'[^']*_bot'", f",'{script}'", rest)
    return f"({entry},{parts[0]},{parts[1]},{rest})"


def main() -> None:
    creature_rows = parse_creature_rows(TEMPLATE_SQL)
    extras = parse_extras(BASE_NPCBOTS)
    appearance = parse_appearance(BASE_NPCBOTS)
    equip_map = parse_equip(EQUIP_SQL)
    prototypes = pick_prototypes(creature_rows, extras, appearance, equip_map)

    used_names: set[str] = set()
    creature_values: list[str] = []
    extras_values: list[str] = []
    appearance_values: list[str] = []
    equip_values: list[str] = []

    entry = ENTRY_FIRST
    for bot_class, _class_label, script in WANDER_CLASSES:
        subname = SUBNAME_BY_CLASS.get(bot_class, "")
        proto_list = prototypes[bot_class]
        for i in range(BOTS_PER_CLASS):
            proto_entry = proto_list[i % len(proto_list)]
            name = make_name(used_names)
            creature_values.append(
                clone_creature_row(creature_rows[proto_entry], entry, name, subname, script)
            )
            cls, race = extras[proto_entry]
            extras_values.append(f"({entry},{cls},{race})")
            if proto_entry in appearance:
                g, skin, face, hair, haircolor, features = appearance[proto_entry]
                appearance_values.append(
                    f"({entry},'{name}',{g},{skin},{face},{hair},{haircolor},{features})"
                )
            else:
                appearance_values.append(f"({entry},'{name}',0,0,0,0,0,0)")
            e1, e2, e3 = equip_map.get(proto_entry, (0, 0, 0))
            equip_values.append(f"({entry},1,{e1},{e2},{e3},-1)")
            entry += 1

    assert entry == ENTRY_LAST + 1, entry

    lines = [
        "-- Expand hireable NPCBot templates for 2000 wandering bots + large raid hire pools.",
        "-- Generated by sql/custom/tools/generate_npcbot_template_expansion.py",
        "-- Apply to world DB, then set NpcBot.WanderingBots.Continents.Count = 2000 and rebuild worldserver (BOT_ENTRY_CREATE_BEGIN = 73000).",
        "",
        f"DELETE FROM `creature_equip_template` WHERE `CreatureID` BETWEEN {ENTRY_FIRST} AND {ENTRY_LAST};",
        f"DELETE FROM `creature_template_npcbot_appearance` WHERE `entry` BETWEEN {ENTRY_FIRST} AND {ENTRY_LAST};",
        f"DELETE FROM `creature_template_npcbot_extras` WHERE `entry` BETWEEN {ENTRY_FIRST} AND {ENTRY_LAST};",
        f"DELETE FROM `creature_template` WHERE `entry` BETWEEN {ENTRY_FIRST} AND {ENTRY_LAST};",
        "",
        "INSERT INTO `creature_template` (`entry`,`difficulty_entry_1`,`difficulty_entry_2`,`difficulty_entry_3`,`KillCredit1`,`KillCredit2`,`modelid1`,`modelid2`,`modelid3`,`modelid4`,`name`,`subname`,`IconName`,`gossip_menu_id`,`minlevel`,`maxlevel`,`exp`,`faction`,`npcflag`,`speed_walk`,`speed_run`,`scale`,`rank`,`dmgschool`,`BaseAttackTime`,`RangeAttackTime`,`BaseVariance`,`RangeVariance`,`unit_class`,`unit_flags`,`unit_flags2`,`dynamicflags`,`family`,`type`,`type_flags`,`lootid`,`pickpocketloot`,`skinloot`,`PetSpellDataId`,`VehicleId`,`mingold`,`maxgold`,`AIName`,`MovementType`,`HoverHeight`,`HealthModifier`,`ManaModifier`,`ArmorModifier`,`DamageModifier`,`ExperienceModifier`,`RacialLeader`,`movementId`,`RegenHealth`,`mechanic_immune_mask`,`spell_school_immune_mask`,`flags_extra`,`ScriptName`,`StringId`,`VerifiedBuild`) VALUES",
    ]
    lines.append(",\n".join(creature_values) + ";")
    lines += [
        "",
        "INSERT INTO `creature_template_npcbot_extras` (`entry`,`class`,`race`) VALUES",
        ",\n".join(extras_values) + ";",
        "",
        "INSERT INTO `creature_template_npcbot_appearance` (`entry`,`name*`,`gender`,`skin`,`face`,`hair`,`haircolor`,`features`) VALUES",
        ",\n".join(appearance_values) + ";",
        "",
        "INSERT INTO `creature_equip_template` (`CreatureID`,`ID`,`ItemID1`,`ItemID2`,`ItemID3`,`VerifiedBuild`) VALUES",
        ",\n".join(equip_values) + ";",
    ]

    OUT_SQL.parent.mkdir(parents=True, exist_ok=True)
    OUT_SQL.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Wrote {OUT_SQL} ({len(creature_values)} templates, entries {ENTRY_FIRST}-{ENTRY_LAST})")


if __name__ == "__main__":
    main()
