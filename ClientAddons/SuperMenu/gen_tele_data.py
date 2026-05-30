# -*- coding: utf-8 -*-
import re
from pathlib import Path

SRC = Path(r"D:\1.TrinityWotlk\Document and Script\SupHeartSrone.lua")
OUT = Path(__file__).parent / "SuperMenu_TeleData.lua"

text = SRC.read_text(encoding="utf-8", errors="replace")

tp_re = re.compile(
    r'\{TP,\s*"([^"]+)"\s*,\s*(-?\d+(?:\.\d+)?)\s*,\s*(-?\d+(?:\.\d+)?)\s*,\s*(-?\d+(?:\.\d+)?)\s*,\s*(-?\d+(?:\.\d+)?)\s*,\s*(-?\d+(?:\.\d+)?)'
)


def clean_name(s: str) -> str:
    s = re.sub(r"\|T[^|]+\|t", "", s)
    s = re.sub(r"\|c[0-9A-Fa-f]{8}", "", s)
    s = s.replace("|r", "")
    return s.strip()


categories = []
for m in re.finditer(r"\[TPMENU\+0x([0-9a-f]+)\]=\{--([^\n]+)\n", text):
    start = m.end()
    rest = text[start:]
    end = len(rest)
    for em in re.finditer(r"\n\t\[(?:TPMENU|MMENU|TBMENU)", rest):
        end = em.start()
        break
    block = rest[:end]
    if "{TP," not in block:
        continue
    cat_name = m.group(2).strip().rstrip("★").strip()
    points = []
    for tp in tp_re.finditer(block):
        label, mapid, x, y, z, o = tp.groups()
        label = clean_name(label)
        if label:
            points.append((label, int(float(mapid)), float(x), float(y), float(z), float(o)))
    if points:
        categories.append((cat_name, points))

lines = [
    "-- 传送点数据（完整摘自 SupHeartSrone.lua，勿删减）",
    "SuperMenu = SuperMenu or {}",
    "",
    "SuperMenu.TeleCategories = {",
]
for cat_name, points in categories:
    lines.append("    {")
    lines.append(f'        name = "{cat_name}",')
    lines.append("        points = {")
    for label, mapid, x, y, z, o in points:
        lines.append(
            f'            {{ "{label}", {mapid}, {x}, {y}, {z}, {o} }},'
        )
    lines.append("        },")
    lines.append("    },")
lines.append("}")
lines.append("")
lines.append("SuperMenu.QuickActions = {")
lines.append('    { "满血", "HEAL" },')
lines.append('    { "脱离战斗", "COMBAT_CLEAR" },')
lines.append('    { "修理装备", "REPAIR" },')
lines.append('    { "解除虚弱", "WEAK" },')
lines.append('    { "在线银行", "BANK" },')
lines.append('    { "空中邮箱", "MAIL" },')
lines.append('    { "重置冷却", "CD_RESET" },')
lines.append('    { "记录炉石点", "BIND_HOME" },')
lines.append('    { "炉石回城", "GO_HOME" },')
lines.append('    { "解除副本绑定", "UNBIND_INST" },')
lines.append('    { "重置天赋", "TALENT_RESET" },')
lines.append('    { "保存角色", "SAVE" },')
lines.append("}")

OUT.write_text("\n".join(lines) + "\n", encoding="utf-8")
total = sum(len(p) for _, p in categories)
print(f"Wrote {len(categories)} categories, {total} teleport points -> {OUT}")
