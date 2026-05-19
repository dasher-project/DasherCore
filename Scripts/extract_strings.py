#!/usr/bin/env python3
"""Extract translatable strings from settings_manifest.json.

Outputs a flat JSON file mapping string keys to English values.
This can be used as the basis for translations.

Usage:
    python3 Scripts/extract_strings.py [--locale en] [--output PATH]

Output format (strings_en.json):
    {
        "BP_DRAW_MOUSE_LINE.label": "Draw Mouse Line",
        "BP_DRAW_MOUSE_LINE.description": "When enabled, a line is drawn...",
        "LP_GEOMETRY.enum.Old Style": "Old Style",
        ...
    }

For translations, create strings_de.json, strings_fr.json, etc. with the same keys.
The app layer loads the appropriate locale file and looks up strings by parameter key.
"""

import json
import os
import argparse


def extract_strings(manifest_path):
    with open(manifest_path, 'r') as f:
        manifest = json.load(f)

    strings = {}
    for p in manifest['parameters']:
        key = p['key']
        if p.get('label'):
            strings[f"{key}.label"] = p['label']
        if p.get('description'):
            strings[f"{key}.description"] = p['description']
        for i, ev in enumerate(p.get('enumValues', [])):
            if ev.get('label'):
                strings[f"{key}.enum.{ev['label']}"] = ev['label']

    return strings


def apply_locale(manifest_path, locale_path):
    """Apply a locale file to produce a localized settings_manifest.json.

    The locale file maps keys like "BP_DRAW_MOUSE_LINE.label" to translated strings.
    Only the label and description fields are overridden — all other metadata stays the same.
    """
    with open(manifest_path, 'r') as f:
        manifest = json.load(f)

    with open(locale_path, 'r') as f:
        locale = json.load(f)

    for p in manifest['parameters']:
        key = p['key']
        label_key = f"{key}.label"
        desc_key = f"{key}.description"

        if label_key in locale:
            p['label'] = locale[label_key]
        if desc_key in locale:
            p['description'] = locale[desc_key]

        for ev in p.get('enumValues', []):
            enum_key = f"{key}.enum.{ev['label']}"
            if enum_key in locale:
                ev['label'] = locale[enum_key]

    return manifest


def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    repo_root = os.path.dirname(script_dir)

    parser = argparse.ArgumentParser(description="Extract or apply locale strings for settings manifest")
    parser.add_argument("--manifest", default=os.path.join(repo_root, "settings_manifest.json"),
                        help="Path to settings_manifest.json")
    parser.add_argument("--locale", default="en",
                        help="Locale code (e.g. en, de, fr)")
    parser.add_argument("--output", default=None,
                        help="Output path (default: Strings/strings_{locale}.json)")
    parser.add_argument("--apply", default=None,
                        help="Path to locale JSON to apply; outputs a localized manifest to --output")
    args = parser.parse_args()

    strings_dir = os.path.join(repo_root, "Strings")
    os.makedirs(strings_dir, exist_ok=True)

    if args.apply:
        manifest = apply_locale(args.manifest, args.apply)
        output = args.output or os.path.join(repo_root, f"settings_manifest_{args.locale}.json")
        with open(output, 'w') as f:
            json.dump(manifest, f, indent=2, ensure_ascii=False)
        print(f"Applied {args.apply} → {output}")
    else:
        strings = extract_strings(args.manifest)
        output = args.output or os.path.join(strings_dir, f"strings_{args.locale}.json")
        with open(output, 'w') as f:
            json.dump(strings, f, indent=2, ensure_ascii=False)
        print(f"Extracted {len(strings)} strings → {output}")


if __name__ == "__main__":
    main()
