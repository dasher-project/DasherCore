#!/usr/bin/env python3
"""Normalize colorInfoName tokens in the autoconverted alphabet corpus.

Root cause of dasher-project/DasherCore#69 (flat/unweighted WA alphabets):
the autoconverter emitted free-form colorInfoName values ("lower case",
"korean trailing consonants", pinyin syllables, ...) but the shipped colour
palettes define exactly eleven tokens:

    accents  limitedPunctuation  lowercase  lowercaseBackground  numbers
    paragraph  paragraphSpace  punctuation  punctuationLong  space  uppercase

A group whose token no palette defines gets no colour information, and the
renderer then draws nothing for its nodes — the alphabet's probabilities
exist but are invisible: a flat, unweighted-looking canvas. 321 of the 323
languages in the catalogue are served only by these alphabets.

Rule (mirror this in the WorldAlphabets Dasher exporter):
  - emit ONLY one of the eleven palette tokens above (or omit the attribute
    entirely — the engine then defaults the group to "lowercase");
  - never free-form group names.

This script rewrites every autoConverted file in place: recognised tokens
are preserved; everything else is mapped by keyword (upper→uppercase,
digits/numerals→numbers, punctuation→punctuation, accents/diacritics/
combining→accents, editing→punctuation, anything else→lowercase).

Usage: python3 Scripts/fix-wa-colorinfo.py [--dry-run]
"""

import re
import sys
from collections import Counter
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
CORPUS = REPO / "Data" / "alphabets" / "autoConverted"

PALETTE_TOKENS = {
    "accents", "limitedPunctuation", "lowercase", "lowercaseBackground",
    "numbers", "paragraph", "paragraphSpace", "punctuation",
    "punctuationLong", "space", "uppercase",
}


def map_token(tok: str) -> str:
    if tok in PALETTE_TOKENS or tok == "":
        return tok
    t = tok.lower()
    if "upper" in t or t.startswith("uc ") or "duże" in t:
        return "uppercase"
    if "lower" in t or t.startswith("lc ") or "small" in t:
        return "lowercase"
    if "digit" in t or "number" in t or "numeral" in t or "liczby" in t:
        return "numbers"
    if "punct" in t or "interpunkcj" in t:
        return "punctuation"
    if "paragraph" in t:
        return "paragraphSpace"
    if "editing" in t:
        return "punctuation"  # action keys (⌫⌦): visually the punctuation bucket
    if "accent" in t or "diacr" in t or "combining" in t or "vowel sign" in t or "modifier" in t:
        return "accents"
    # Everything else — script-specific letter groups, syllabary rows — is an
    # ordinary letter group.
    return "lowercase"


def main() -> int:
    dry = "--dry-run" in sys.argv
    changes = Counter()
    files_changed = 0
    for path in sorted(CORPUS.glob("alphabet*.xml")):
        text = path.read_text(encoding="utf-8")
        def repl(m):
            old, new = m.group(1), map_token(m.group(1))
            if old != new:
                changes[f"{old} → {new}"] += 1
            return f'colorInfoName="{new}"'
        new_text = re.sub(r'colorInfoName="([^"]*)"', repl, text)
        if new_text != text:
            files_changed += 1
            if not dry:
                path.write_text(new_text, encoding="utf-8")

    print(f"{'would rewrite' if dry else 'rewrote'} {files_changed} files")
    for change, n in changes.most_common(20):
        print(f"  {n:4d}  {change}")
    if len(changes) > 20:
        print(f"  ... and {len(changes) - 20} more mappings")
    return 0


if __name__ == "__main__":
    sys.exit(main())
