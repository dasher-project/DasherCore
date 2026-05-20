#!/usr/bin/env python3
"""Generate DasherCore alphabet XML files from WorldAlphabets data.

Usage:
    python3 Scripts/generate_alphabets.py [--source DIR] [--output DIR]

Defaults:
    --source   WorldAlphabets data/alphabets directory
    --output   DasherCore Data/alphabets/autoConverted directory

Reads each alphabet JSON from WorldAlphabets and produces a DasherCore
alphabet XML in the new format (node-based, per alphabet.dtd).

The script also generates a training frequency file for each alphabet
based on WorldAlphabets frequency data.
"""

import json
import os
import sys
import argparse
import xml.etree.ElementTree as ET

SCRIPT_TO_ORIENTATION = {
    "Arab": "RL",
    "Hebr": "RL",
    "Syrc": "RL",
    "Nkoo": "RL",
    "Thaa": "RL",
    "Rohg": "RL",
    "Adlm": "RL",
}

SCRIPT_TO_ENCODING = {
    "Arab": "Arabic",
    "Hebr": "Hebrew",
    "Syrc": "Arabic",
    "Cyrl": "Cyrillic",
    "Grek": "Greek",
    "Deva": "CentralEurope",
    "Beng": "CentralEurope",
    "Gujr": "CentralEurope",
    "Guru": "CentralEurope",
    "Knda": "CentralEurope",
    "Mlym": "CentralEurope",
    "Orya": "CentralEurope",
    "Taml": "CentralEurope",
    "Telu": "CentralEurope",
    "Sinh": "CentralEurope",
    "Tibt": "CentralEurope",
    "Mymr": "Thai",
    "Thai": "Thai",
    "Geor": "CentralEurope",
    "Armn": "CentralEurope",
    "Hang": "Korean",
    "Kore": "Korean",
    "Hira": "Japanese",
    "Kana": "Japanese",
    "Jpan": "Japanese",
    "Hans": "ChineseSimplified",
    "Hant": "ChineseTraditional",
    "Laoo": "Thai",
    "Khmr": "Thai",
    "Ethi": "CentralEurope",
    "Cans": "Western",
    "Cher": "Western",
    "Copt": "Western",
    "Vaii": "Western",
    "Yiii": "Western",
    "Olck": "Western",
    "Saur": "CentralEurope",
    "Lepc": "CentralEurope",
    "Lisu": "CentralEurope",
    "Mtei": "CentralEurope",
    "Mong": "CentralEurope",
    "Bamu": "Western",
    "Nkoo": "Arabic",
    "Osge": "Western",
    "Rohg": "Arabic",
    "Adlm": "Arabic",
    "Cakm": "CentralEurope",
}

LANGCODE_TO_ORIENTATION = {
    "fa": "RL",
    "ur": "RL",
    "ps": "RL",
    "sd": "RL",
    "ckb": "RL",
    "yi": "RL",
}


def xml_escape(text):
    return text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;").replace('"', "&quot;")


def make_node(label, text=None, fixed_probability=None):
    attrs = {"label": label}
    if text is not None and text != label:
        attrs["text"] = text
    if fixed_probability is not None:
        attrs["fixedProbability"] = str(fixed_probability)
    node = ET.Element("node", attrs)
    ET.SubElement(node, "textCharAction")
    return node


def make_delete_node(label="⌫", distance="last char"):
    node = ET.Element("node", {"label": label})
    ET.SubElement(node, "deleteTextAction", {"distance": distance})
    return node


def alphabet_filename(lang_name, iso_code, script):
    safe = lang_name.lower().replace(" ", ".").replace(",", "").replace("(", "").replace(")", "")
    safe = "".join(c for c in safe if c.isalnum() or c == ".")
    safe = safe.replace("..", ".")
    return f"alphabet.wa.{safe}.{iso_code}-{script}.xml"


def training_filename(iso_code, script):
    return f"training_wa_{iso_code}_{script}.txt"


def generate_training_text(freq_data, all_chars, iso_code, script):
    lines = []
    lines.append(f"# Training text for {iso_code}/{script}")
    lines.append(f"# Auto-generated from WorldAlphabets frequency data")
    lines.append("")

    if isinstance(freq_data, dict):
        sorted_chars = sorted(freq_data.items(), key=lambda x: -x[1])
        for char, freq in sorted_chars:
            repeat = max(1, int(freq * 1000))
            lines.append((char + " ") * min(repeat, 50))
    elif isinstance(freq_data, list) and freq_data:
        for entry in freq_data:
            if isinstance(entry, dict):
                char = entry.get("ch", "")
                repeat = max(1, int(entry.get("freq", 0) * 1000))
                lines.append((char + " ") * min(repeat, 50))

    for char in all_chars:
        lines.append(char)

    return "\n".join(lines)


def generate_alphabet(data, iso_code, script):
    lang_name = data.get("language", iso_code)
    uppercase = data.get("uppercase", [])
    lowercase = data.get("lowercase", [])
    alphabetical = data.get("alphabetical", [])
    digits = data.get("digits", [])
    freq = data.get("frequency", {})

    orientation = SCRIPT_TO_ORIENTATION.get(script, "LR")
    orientation = LANGCODE_TO_ORIENTATION.get(iso_code, orientation)

    train_file = training_filename(iso_code, script)

    root = ET.Element("alphabet", {
        "name": f"{lang_name} (WorldAlphabets)",
        "orientation": orientation,
        "trainingFilename": train_file,
        "colorsName": "Default",
    })

    if lowercase:
        group = ET.SubElement(root, "group", {"name": "Lower case", "colorInfoName": "lower case"})
        for ch in lowercase:
            group.append(make_node(ch))

    if uppercase:
        group = ET.SubElement(root, "group", {"name": "Upper case", "colorInfoName": "upper case"})
        for ch in uppercase:
            group.append(make_node(ch))

    if not lowercase and not uppercase and alphabetical:
        group = ET.SubElement(root, "group", {"name": "Characters", "colorInfoName": "characters"})
        for ch in alphabetical:
            group.append(make_node(ch))

    if digits:
        group = ET.SubElement(root, "group", {"name": "Digits", "colorInfoName": "digits"})
        for ch in digits:
            group.append(make_node(ch))

    control_group = ET.SubElement(root, "group", {"name": "paragraphSpace", "colorInfoName": "paragraphSpace"})
    control_group.append(make_node("¶", "\n"))
    control_group.append(make_node("␣", " "))

    edit_group = ET.SubElement(root, "group", {"name": "Editing", "colorInfoName": "editing"})
    edit_group.append(make_delete_node("⌫", "last char"))
    edit_group.append(make_delete_node("⌦", "next char"))
    edit_group.append(make_delete_node("⌫ Word", "last word"))

    all_chars = lowercase + uppercase + digits + alphabetical
    return root, all_chars


def write_xml(root, filepath):
    tree = ET.ElementTree(root)
    ET.indent(tree, space="  ", level=0)
    with open(filepath, "wb") as f:
        f.write(b'<?xml version="1.0" encoding="UTF-8"?>\n')
        f.write(b'<!DOCTYPE alphabet SYSTEM "../alphabet.dtd">\n')
        tree.write(f, encoding="utf-8", xml_declaration=False)


def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    repo_root = os.path.dirname(script_dir)

    parser = argparse.ArgumentParser(description="Generate DasherCore alphabet XMLs from WorldAlphabets")
    parser.add_argument("--source", default=os.path.join(repo_root, "..", "WorldAlphabets", "data", "alphabets"),
                        help="WorldAlphabets data/alphabets directory")
    parser.add_argument("--output", default=os.path.join(repo_root, "Data", "alphabets", "autoConverted"),
                        help="DasherCore output directory")
    parser.add_argument("--training-output", default=os.path.join(repo_root, "Data", "training"),
                        help="Training text output directory")
    parser.add_argument("--filter", default=None,
                        help="Only generate for codes matching this prefix (e.g. 'en', 'ar')")
    args = parser.parse_args()

    os.makedirs(args.output, exist_ok=True)
    os.makedirs(args.training_output, exist_ok=True)

    generated = 0
    skipped = 0
    errors = 0

    for filename in sorted(os.listdir(args.source)):
        if not filename.endswith(".json"):
            continue

        code_script = filename.replace(".json", "")
        parts = code_script.split("-", 1)
        if len(parts) != 2:
            continue
        iso_code, script = parts

        if args.filter and not iso_code.startswith(args.filter):
            continue

        filepath = os.path.join(args.source, filename)
        try:
            with open(filepath, encoding="utf-8") as f:
                data = json.load(f)
        except Exception as e:
            print(f"  ERROR reading {filename}: {e}")
            errors += 1
            continue

        alphabetical = data.get("alphabetical", [])
        lowercase = data.get("lowercase", [])
        uppercase = data.get("uppercase", [])

        if not alphabetical and not lowercase and not uppercase:
            skipped += 1
            continue

        lang_name = data.get("language", iso_code)

        try:
            root, all_chars = generate_alphabet(data, iso_code, script)

            out_filename = alphabet_filename(lang_name, iso_code, script)
            out_path = os.path.join(args.output, out_filename)
            write_xml(root, out_path)

            freq = data.get("frequency", {})
            if freq:
                train_path = os.path.join(args.training_output, training_filename(iso_code, script))
                train_text = generate_training_text(freq, all_chars, iso_code, script)
                with open(train_path, "w", encoding="utf-8") as f:
                    f.write(train_text)

            generated += 1
            print(f"  {out_filename} ({len(all_chars)} chars)")
        except Exception as e:
            print(f"  ERROR generating {filename}: {e}")
            errors += 1

    print(f"\nDone: {generated} generated, {skipped} skipped (empty), {errors} errors")


if __name__ == "__main__":
    main()
