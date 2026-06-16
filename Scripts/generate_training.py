#!/usr/bin/env python3
"""Generate DasherCore training text files from AAC conversational datasets.

Uses the KeithAAC multilingual datasets (COMM2-style dialogues, Imagine, Turk,
DailyDialog) which contain real conversational text — the ideal training input
for Dasher's PPM language model.

Also generates alphabet XML files from WorldAlphabets data.

Usage:
    python3 Scripts/generate_training.py \\
        --keith ~/GitHub/DasherProjects/keithdata \\
        --worldalphabets ~/GitHub/WorldAlphabets/data/alphabets \\
        --output Data/training \\
        --alphabets Data/alphabets/autoConverted

The training files are plain text: one utterance per line.
The alphabet files are DasherCore XML (new node-based format).
"""

import json
import os
import sys
import argparse
import xml.etree.ElementTree as ET


# ── Keith AAC dataset extraction ──────────────────────────────────────────

KEITH_DATASETS = ["imagine", "turk", "daily"]
KEITH_SPLITS = ["train", "dev"]

LANG_TO_KEITH = {
    "af": "af", "ar": "ar", "bn": "bn", "ca": "ca", "cs": "cs", "cy": "cy",
    "da": "da", "de": "de", "el": "el", "en": "en", "es": "es", "eu": "eu",
    "fa": "fa", "fi": "fi", "fr": "fr", "he": "he", "hr": "hr", "it": "it",
    "ja": "ja", "mn": "mn", "nl": "nl", "no": "no", "pl": "pl", "pt": "pt",
    "ru": "ru", "sk": "sk", "sl": "sl", "sq": "sq", "sv": "sv", "sw": "sw",
    "tr": "tr", "uk": "uk", "zh-CN": "zh-CN", "zh-TW": "zh-TW",
}


def extract_utterances(keith_dir, lang_code):
    keith_lang = LANG_TO_KEITH.get(lang_code)
    if not keith_lang:
        return []

    lines = []
    for dataset in KEITH_DATASETS:
        for split in KEITH_SPLITS:
            if dataset == "daily":
                prefix = "daily"
            elif dataset == "imagine":
                prefix = "imagine"
            elif dataset == "turk":
                prefix = "turk"

            filepath = os.path.join(keith_dir, dataset, keith_lang, f"{prefix}_{split}.json")
            if not os.path.exists(filepath):
                continue

            try:
                with open(filepath, encoding="utf-8") as f:
                    data = json.load(f)
            except Exception:
                continue

            if not isinstance(data, list):
                continue

            for entry in data:
                if "turns" in entry:
                    for turn in entry["turns"]:
                        text = turn.get("turn", "").strip()
                        if text:
                            lines.append(text)
                elif "text" in entry:
                    text = entry["text"].strip()
                    if text:
                        lines.append(text)

    return lines


# ── Dasher training file format ───────────────────────────────────────────

def write_training_file(lines, filepath):
    with open(filepath, "w", encoding="utf-8") as f:
        for line in lines:
            f.write(line + "\n")


# ── WorldAlphabets alphabet generation ────────────────────────────────────

SCRIPT_TO_ORIENTATION = {
    "Arab": "RL", "Hebr": "RL", "Syrc": "RL", "Nkoo": "RL",
    "Thaa": "RL", "Rohg": "RL", "Adlm": "RL",
}

LANG_TO_ORIENTATION = {
    "fa": "RL", "ur": "RL", "ps": "RL", "sd": "RL", "ckb": "RL", "yi": "RL",
}


def make_node(label, text=None):
    attrs = {"label": label}
    if text is not None and text != label:
        attrs["text"] = text
    node = ET.Element("node", attrs)
    ET.SubElement(node, "textCharAction")
    return node


def make_delete_node(label, distance):
    node = ET.Element("node", {"label": label})
    ET.SubElement(node, "deleteTextAction", {"distance": distance})
    return node


def generate_alphabet(data, iso_code, script, train_file):
    lang_name = data.get("language", iso_code)
    uppercase = data.get("uppercase", [])
    lowercase = data.get("lowercase", [])
    alphabetical = data.get("alphabetical", [])
    digits = data.get("digits", [])

    orientation = SCRIPT_TO_ORIENTATION.get(script, "LR")
    orientation = LANG_TO_ORIENTATION.get(iso_code, orientation)

    root = ET.Element("alphabet", {
        "name": f"{lang_name} (WorldAlphabets)",
        "orientation": orientation,
        "trainingFilename": train_file,
        "colorsName": "Default",
    })

    if lowercase:
        g = ET.SubElement(root, "group", {"name": "Lower case", "colorInfoName": "lower case"})
        for ch in lowercase:
            g.append(make_node(ch))

    if uppercase:
        g = ET.SubElement(root, "group", {"name": "Upper case", "colorInfoName": "upper case"})
        for ch in uppercase:
            g.append(make_node(ch))

    if not lowercase and not uppercase and alphabetical:
        g = ET.SubElement(root, "group", {"name": "Characters", "colorInfoName": "characters"})
        for ch in alphabetical:
            g.append(make_node(ch))

    if digits:
        g = ET.SubElement(root, "group", {"name": "Digits", "colorInfoName": "digits"})
        for ch in digits:
            g.append(make_node(ch))

    ctrl = ET.SubElement(root, "group", {"name": "paragraphSpace", "colorInfoName": "paragraphSpace"})
    ctrl.append(make_node("¶", "\n"))
    ctrl.append(make_node("␣", " "))

    edit = ET.SubElement(root, "group", {"name": "Editing", "colorInfoName": "editing"})
    edit.append(make_delete_node("⌫", "last char"))
    edit.append(make_delete_node("⌦", "next char"))
    edit.append(make_delete_node("⌫ Word", "last word"))

    return root


def write_alphabet_xml(root, filepath):
    tree = ET.ElementTree(root)
    ET.indent(tree, space="  ", level=0)
    with open(filepath, "wb") as f:
        f.write(b'<?xml version="1.0" encoding="UTF-8"?>\n')
        f.write(b'<!DOCTYPE alphabet SYSTEM "../alphabet.dtd">\n')
        tree.write(f, encoding="utf-8", xml_declaration=False)


# ── Main ───────────────────────────────────────────────────────────────────

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    repo_root = os.path.dirname(script_dir)

    parser = argparse.ArgumentParser(description="Generate DasherCore training + alphabet files")
    parser.add_argument("--keith", default=os.path.join(repo_root, "..", "keithdata"),
                        help="Path to keithdata directory")
    parser.add_argument("--worldalphabets", default=os.path.join(repo_root, "..", "WorldAlphabets", "data", "alphabets"),
                        help="Path to WorldAlphabets data/alphabets")
    parser.add_argument("--output", default=os.path.join(repo_root, "Data", "training"),
                        help="Training text output directory")
    parser.add_argument("--alphabets", default=os.path.join(repo_root, "Data", "alphabets", "autoConverted"),
                        help="Alphabet XML output directory")
    parser.add_argument("--filter", default=None,
                        help="Only generate for codes matching prefix (e.g. 'en', 'ar')")
    args = parser.parse_args()

    os.makedirs(args.output, exist_ok=True)
    os.makedirs(args.alphabets, exist_ok=True)

    train_generated = 0
    alph_generated = 0
    errors = 0

    wa_dir = args.worldalphabets if os.path.isdir(args.worldalphabets) else None

    for filename in sorted(os.listdir(args.worldalphabets if wa_dir else args.alphabets)):
        if wa_dir:
            if not filename.endswith(".json"):
                continue
            code_script = filename.replace(".json", "")
            parts = code_script.split("-", 1)
            if len(parts) != 2:
                continue
            iso_code, script = parts
        else:
            continue

        if args.filter and not iso_code.startswith(args.filter):
            continue

        train_filename = f"training_wa_{iso_code}_{script}.txt"

        utterances = extract_utterances(args.keith, iso_code)

        if utterances:
            train_path = os.path.join(args.output, train_filename)
            write_training_file(utterances, train_path)
            train_generated += 1
            print(f"  {train_filename} ({len(utterances)} utterances)")

        if wa_dir:
            wa_path = os.path.join(args.worldalphabets, filename)
            try:
                with open(wa_path, encoding="utf-8") as f:
                    data = json.load(f)

                alphabetical = data.get("alphabetical", [])
                lowercase = data.get("lowercase", [])
                uppercase = data.get("uppercase", [])
                if not alphabetical and not lowercase and not uppercase:
                    continue

                root = generate_alphabet(data, iso_code, script, train_filename if utterances else "")
                safe_name = data.get("language", iso_code).lower().replace(" ", ".")
                safe_name = "".join(c for c in safe_name if c.isalnum() or c == ".")
                out_name = f"alphabet.wa.{safe_name}.{iso_code}-{script}.xml"
                write_alphabet_xml(root, os.path.join(args.alphabets, out_name))
                alph_generated += 1
            except Exception as e:
                print(f"  ERROR {filename}: {e}")
                errors += 1

    print(f"\nDone: {train_generated} training files, {alph_generated} alphabets, {errors} errors")


if __name__ == "__main__":
    main()
