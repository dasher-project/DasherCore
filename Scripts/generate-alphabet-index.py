#!/usr/bin/env python3
"""Generate the Dasher alphabet index (alphabet_index.json).

Parses every alphabet.*.xml under Data/alphabets/ (full XML parse — this runs
offline, not at engine startup) and emits one JSON artifact with per-alphabet
metadata:

  id          — the AlphID (root <alphabet name="...">), the engine's key
  file        — path relative to Data/alphabets/
  orientation — ltr | rtl | ttb | btt   (from the orientation attribute)
  lang        — best-effort BCP-47 language code (see derivation order)
  script      — ISO 15924 script code (Latn, Arab, Cyrl, ...)
  script_name — human-readable script name
  training    — training corpus filename, when the alphabet declares one
  palette     — preferred colour palette name
  conversion  — none | mandarin | routingContextInsensitive | routingContextSensitive
  chars       — number of symbols in the alphabet
  groups      — group display names
  source      — maintained | worldalphabets | legacy   (which corpus tier)

Lang/script derivation order:
  1. WorldAlphabets filename pattern:  alphabet.wa.<desc>.<lang>-<Script>.xml
  2. WorldAlphabets training filename: training_wa_<lang>_<Script>.txt
  3. Legacy training filename:         training_<language>_<REGION>.txt (+ name map)
  4. Curated overrides for the maintained/legacy alphabets (LANG_OVERRIDES)
  5. Script fallback from Unicode script detection (script only, lang null)

Consumers: the Dasher website's alphabet catalogue, frontends that want
metadata-driven settings (e.g. auto-RTL layout), and humans. The engine's
runtime name index stays independent (CAlphIO::ScanNameIndex) so a missing
or stale generated file never breaks startup.

Usage:
    python3 Scripts/generate-alphabet-index.py   # writes Data/alphabets/alphabet_index.json
"""

import json
import re
import sys
import time
import xml.etree.ElementTree as ET
from collections import Counter
from pathlib import Path
REPO = Path(__file__).resolve().parent.parent
ALPHABETS_DIR = REPO / "Data" / "alphabets"
OUT_FILE = ALPHABETS_DIR / "alphabet_index.json"

# ── Unicode script detection ────────────────────────────────────────────────
# (range start, range end, ISO 15924, display name). Order matters only for
# readability; votes are counted per character.
SCRIPT_RANGES = [
    (0x0041, 0x005A, "Latn", "Latin"),
    (0x0061, 0x007A, "Latn", "Latin"),
    (0x00C0, 0x024F, "Latn", "Latin"),
    (0x1E00, 0x1EFF, "Latn", "Latin"),
    (0x2C60, 0x2C7F, "Latn", "Latin"),
    (0xA720, 0xA7FF, "Latn", "Latin"),
    (0x0530, 0x058F, "Armn", "Armenian"),
    (0x0590, 0x05FF, "Hebr", "Hebrew"),
    (0x0600, 0x06FF, "Arab", "Arabic"),
    (0x0750, 0x077F, "Arab", "Arabic"),
    (0xFB50, 0xFDFF, "Arab", "Arabic"),
    (0xFE70, 0xFEFF, "Arab", "Arabic"),
    (0x0700, 0x074F, "Syrc", "Syriac"),
    (0x0780, 0x07BF, "Thaa", "Thaana"),
    (0x07C0, 0x07FF, "Nkoo", "N'Ko"),
    (0x0900, 0x097F, "Deva", "Devanagari"),
    (0x0980, 0x09FF, "Beng", "Bengali"),
    (0x0A00, 0x0A7F, "Guru", "Gurmukhi"),
    (0x0A80, 0x0AFF, "Gujr", "Gujarati"),
    (0x0B00, 0x0B7F, "Orya", "Oriya"),
    (0x0B80, 0x0BFF, "Taml", "Tamil"),
    (0x0C00, 0x0C7F, "Telu", "Telugu"),
    (0x0C80, 0x0CFF, "Knda", "Kannada"),
    (0x0D00, 0x0D7F, "Mlym", "Malayalam"),
    (0x0D80, 0x0DFF, "Sinh", "Sinhala"),
    (0x0E00, 0x0E7F, "Thai", "Thai"),
    (0x0E80, 0x0EFF, "Laoo", "Lao"),
    (0x0F00, 0x0FFF, "Tibt", "Tibetan"),
    (0x1000, 0x109F, "Mymr", "Myanmar"),
    (0x10A0, 0x10FF, "Geor", "Georgian"),
    (0x1100, 0x11FF, "Hang", "Hangul"),
    (0x3130, 0x318F, "Hang", "Hangul"),
    (0xA960, 0xA97F, "Hang", "Hangul"),
    (0xAC00, 0xD7FF, "Hang", "Hangul"),
    (0x1200, 0x137F, "Ethi", "Ethiopic"),
    (0x13A0, 0x13FF, "Cher", "Cherokee"),
    (0x1400, 0x167F, "Cans", "Canadian Aboriginal"),
    (0x1680, 0x169F, "Ogam", "Ogham"),
    (0x16A0, 0x16FF, "Runr", "Runic"),
    (0x1780, 0x17FF, "Khmr", "Khmer"),
    (0x1800, 0x18AF, "Mong", "Mongolian"),
    (0x2D30, 0x2D7F, "Tfng", "Tifinagh"),
    (0x3040, 0x309F, "Hira", "Hiragana"),
    (0x30A0, 0x30FF, "Kana", "Katakana"),
    (0x31F0, 0x31FF, "Kana", "Katakana"),
    (0x4E00, 0x9FFF, "Hani", "Han"),
    (0x3400, 0x4DBF, "Hani", "Han"),
    (0xF900, 0xFAFF, "Hani", "Han"),
    (0x3100, 0x312F, "Bopo", "Bopomofo"),
    (0x0400, 0x04FF, "Cyrl", "Cyrillic"),
    (0x0500, 0x052F, "Cyrl", "Cyrillic"),
    (0x0370, 0x03FF, "Grek", "Greek"),
    (0x1F00, 0x1FFF, "Grek", "Greek"),
]

# Characters that carry no script signal (ASCII punctuation/digits/space,
# combining marks, symbols). The vote ignores them.
NEUTRAL_RANGES = [
    (0x0000, 0x0040),   # controls, space, punctuation, digits
    (0x005B, 0x00BF),   # ASCII punctuation/symbols
    (0x0300, 0x036F),   # combining diacritics
    (0x2000, 0x2BFF),   # general punctuation, symbols, arrows, dingbats
    (0xFE00, 0xFE0F),   # variation selectors
    (0xFFF0, 0xFFFF),
    (0x25A0, 0x25FF),   # geometric shapes (Dasher uses ◌ U+25CC etc.)
]


def char_script(cp):
    for lo, hi in NEUTRAL_RANGES:
        if lo <= cp <= hi:
            return None
    for lo, hi, code, name in SCRIPT_RANGES:
        if lo <= cp <= hi:
            return (code, name)
    return None


def detect_script(texts):
    """Majority vote over script-bearing characters in the symbol texts."""
    votes = Counter()
    for t in texts:
        for ch in t:
            s = char_script(ord(ch))
            if s:
                votes[s] += 1
    if not votes:
        return None, None
    (code, name), _ = votes.most_common(1)[0]
    return code, name


# ── Language derivation ─────────────────────────────────────────────────────
WA_FILENAME_RE = re.compile(r"\.wa\..*\.([a-z]{2,3})-([A-Z][a-z]{3})\.xml$")
WA_TRAINING_RE = re.compile(r"^training_wa_([a-z]{2,3})_([A-Z][a-z]{3})\.txt$")
LEGACY_TRAINING_RE = re.compile(r"^training_([a-z]+)_([A-Z]{2})\.txt$")

# Legacy training basenames → BCP-47 (curated; covers the maintained set)
LEGACY_TRAINING_LANG = {
    "english": "en",
    "deutsch": "de",
    "ukenglish": "en-GB",
}

# Curated overrides keyed by AlphID for alphabets whose filenames/training
# files carry no language signal (maintained + notable legacy alphabets).
LANG_OVERRIDES = {
    "English with limited punctuation": "en",
    "English with numerals and limited punctuation": "en",
    "English with numerals and lots of punctuation": "en",
    "English without punctuation": "en",
    "English lower case": "en",
    "English with accents, numerals & punctuation": "en",
    "English (UK)": "en-GB",
    "English LaTeX": "en",
    "Deutsch (German) with limited punctuation": "de",
    "Deutsch (German) with numerals and punctuation": "de",
    "Japanese Canna": "ja",
    "Mandarin (simp) by Pinyin, tone after first vowel": "zh-Hans",
    "Mandarin (simp) by Pinyin, tone last": "zh-Hans",
    "Mandarin (simp) by Pinyin, tones as written": "zh-Hans",
    "Mandarin (trad) via ㄅㄆㄇㄈ (Bopomofo)": "zh-Hant",
    "Urdu with punctuation and numerals": "ur",
}

# Language-name tokens → BCP-47, matched against legacy filename words when
# nothing more authoritative fired. Longest/most-specific first.
TOKEN_LANG = [
    ("ukenglish", "en-GB"), ("deutsch", "de"), ("german", "de"),
    ("adangbe", "adq"), ("afrikaans", "af"), ("akan", "ak"), ("albanian", "sq"),
    ("amharic", "am"), ("arabic", "ar"), ("armenian", "hy"), ("assamese", "as"),
    ("avar", "av"), ("awaadhi", "awa"), ("azerbaijani", "az"), ("azeri", "az"),
    ("baluchi", "bal"), ("basque", "eu"), ("belarusian", "be"), ("bengali", "bn"),
    ("bhojpuri", "bho"), ("bosnian", "bs"), ("bosanski", "bs"), ("breton", "br"),
    ("bulgarian", "bg"), ("burmese", "my"), ("cantonese", "yue"),
    ("catalan", "ca"), ("cebuano", "ceb"), ("cherokee", "chr"), ("chichewa", "ny"),
    ("chinese", "zh"), ("corsican", "co"), ("croatian", "hr"), ("czech", "cs"),
    ("danish", "da"), ("dhivehi", "dv"), ("dutch", "nl"), ("english", "en"),
    ("esperanto", "eo"), ("estonian", "et"), ("ewe", "ee"), ("faroese", "fo"),
    ("farsi", "fa"), ("persian", "fa"), ("finnish", "fi"), ("french", "fr"),
    ("frisian", "fy"), ("fulah", "ff"), ("gaidhlig", "gd"), ("scots.gaelic", "gd"),
    ("galician", "gl"), ("georgian", "ka"), ("greek", "el"), ("greenlandic", "kl"),
    ("guarani", "gn"), ("gujarati", "gu"), ("hausa", "ha"), ("hawaiian", "haw"),
    ("hebrew", "he"), ("hiragana", "ja"), ("katakana", "ja"), ("kana", "ja"),
    ("hungarian", "hu"), ("icelandic", "is"),
    ("igbo", "ig"), ("indonesian", "id"), ("inuktitut", "iu"), ("irish", "ga"),
    ("italian", "it"), ("japanese", "ja"), ("kannada", "kn"), ("kashmiri", "ks"),
    ("kazakh", "kk"), ("khmer", "km"), ("khasi", "kha"), ("kirghiz", "ky"),
    ("kirundi", "rn"), ("rundi", "rn"), ("kongo", "kg"),
    ("korean", "ko"), ("kurdish", "ku"), ("lao", "lo"), ("latin", "la"),
    ("latvian", "lv"), ("lingala", "ln"), ("lithuanian", "lt"),
    ("luxembourgish", "lb"), ("macedonian", "mk"), ("magahi", "mag"),
    ("malagasy", "mg"), ("malay", "ms"), ("malayalam", "ml"), ("maltese", "mt"),
    ("maori", "mi"), ("marathi", "mr"), ("mongolian", "mn"), ("mongol", "mn"),
    ("nepali", "ne"), ("norwegian", "no"), ("occitan", "oc"), ("oromo", "om"),
    ("pashto", "ps"), ("polish", "pl"), ("portuguese", "pt"), ("punjabi", "pa"),
    ("rajasthani", "raj"), ("romanian", "ro"), ("romansh", "rm"),
    ("russian", "ru"), ("sanskrit", "sa"), ("serbian", "sr"), ("sesotho", "st"),
    ("shona", "sn"), ("sindhi", "sd"), ("sinhala", "si"), ("slovak", "sk"),
    ("slovenian", "sl"), ("somali", "so"), ("sorani", "ckb"), ("spanish", "es"),
    ("sundanese", "su"), ("swahili", "sw"), ("swati", "ss"), ("swedish", "sv"),
    ("tagalog", "tl"), ("tahitian", "ty"), ("tajik", "tg"), ("tamil", "ta"),
    ("tatar", "tt"), ("telugu", "te"), ("thai", "th"), ("tibetan", "bo"),
    ("tigrinya", "ti"), ("tonga", "to"), ("tsonga", "ts"), ("tswana", "tn"),
    ("turkish", "tr"), ("turkmen", "tk"), ("ukrainian", "uk"), ("urdu", "ur"),
    ("uyghur", "ug"), ("uzbek", "uz"), ("vietnamese", "vi"), ("welsh", "cy"),
    ("wolof", "wo"), ("xhosa", "xh"), ("yiddish", "yi"), ("yoruba", "yo"),
    ("zulu", "zu"),
]

ORIENTATION_MAP = {"LR": "ltr", "RL": "rtl", "TB": "ttb", "BT": "btt"}
SOURCE_BY_DIR = {
    ".": "maintained",
    "autoConverted": "worldalphabets",
    "oldAlphabets": "legacy",
}


def derive_lang_and_script(file_name, training, alph_id, texts):
    m = WA_FILENAME_RE.search(file_name)
    if m:
        return m.group(1), m.group(2), None
    if training:
        m = WA_TRAINING_RE.match(training)
        if m:
            return m.group(1), m.group(2), None
        m = LEGACY_TRAINING_RE.match(training)
        if m and m.group(1) in LEGACY_TRAINING_LANG:
            return LEGACY_TRAINING_LANG[m.group(1)], None, None
    if alph_id in LANG_OVERRIDES:
        return LANG_OVERRIDES[alph_id], None, None
    # Legacy filename words: "alphabet.akan.twi.with.lots.of..." → "akan" → ak
    lowered = file_name.lower()
    for token, lang in TOKEN_LANG:
        if token in lowered:
            return lang, None, "token"
    # Then the AlphID itself: "Belarusian with punctuation..." → be
    id_lower = " " + alph_id.lower() + " "
    for token, lang in TOKEN_LANG:
        # match on word boundaries so "ga" doesn't hit "gaelic"-style noise
        if f" {token}" in id_lower:
            return lang, None, "token"
    return None, None, None


def main():
    if not ALPHABETS_DIR.is_dir():
        print(f"error: {ALPHABETS_DIR} not found", file=sys.stderr)
        return 1

    # Corpus availability: which declared training files actually ship.
    training_dir = ALPHABETS_DIR.parent / "training"
    TRAINING_FILES = {p.name for p in training_dir.glob("training_*")} if training_dir.is_dir() else set()

    alphabets = []
    errors = []
    for path in sorted(ALPHABETS_DIR.rglob("alphabet*.xml")):
        rel = path.relative_to(ALPHABETS_DIR)
        if path.name == "alphabet.dtd":
            continue
        try:
            root = ET.parse(path).getroot()
        except ET.ParseError as e:
            errors.append({"file": str(rel), "error": str(e)})
            continue

        # v5 wrapper: <alphabets><alphabet>...</alphabet></alphabets>
        if root.tag == "alphabets":
            root = root.find("alphabet")
            if root is None:
                errors.append({"file": str(rel), "error": "v5 wrapper with no <alphabet>"})
                continue
        if root.tag != "alphabet":
            continue

        alph_id = root.get("name", "").strip()
        if not alph_id:
            errors.append({"file": str(rel), "error": "missing name attribute"})
            continue

        orientation = ORIENTATION_MAP.get(root.get("orientation", "LR"), "ltr")
        training = root.get("trainingFilename", "")
        palette = root.get("colorsName", "")
        conversion = root.get("conversionMode", "none")

        groups = []
        texts = []
        for g in root.iter("group"):
            gname = g.get("name", "")
            if gname and gname not in groups:
                groups.append(gname)
        # Symbols: v6 <node label="..."> (+ optional textCharAction output
        # text) and v5 <s t="..." d="...">. Labels/glyphs drive script
        # detection; output text is the secondary signal.
        for n in root.iter("node"):
            label = n.get("label", "")
            if label:
                texts.append(label)
            ta = n.find("textCharAction")
            if ta is not None and (ta.text or "").strip():
                texts.append(ta.text.strip())
        for s in root.iter("s"):
            for attr in ("t", "d"):
                v = s.get(attr, "")
                if v:
                    texts.append(v)

        lang, lang_script, _ = derive_lang_and_script(path.name, training, alph_id, texts)
        det_script, det_script_name = detect_script(texts)
        script = lang_script or det_script

        script_name = None
        for lo, hi, code, name in SCRIPT_RANGES:
            if code == script:
                script_name = name
                break

        source = SOURCE_BY_DIR.get(str(rel.parent), "legacy")

        entry = {
            "id": alph_id,
            "file": str(rel),
            "orientation": orientation,
            "lang": lang,
            "script": script,
            "script_name": script_name or det_script_name,
            "training": training,
            # Whether the declared corpus file actually ships in Data/training
            # (alphabets without one train uniform until a corpus is imported
            # from WorldAlphabets — Scripts/import-training-from-worldalphabets.py).
            "training_available": bool(training) and training in TRAINING_FILES,
            "palette": palette,
            "conversion": conversion,
            "chars": len(texts),
            "groups": groups,
            "source": source,
        }

        # Flag declared-vs-script direction mismatches (real data bugs in
        # some legacy files — Urdu and Dhivehi declare LR). The engine
        # follows the declared value; the flag lets the website show a
        # warning and lets us fix the XMLs.
        RTL_SCRIPTS = {"Arab", "Hebr", "Thaa", "Nkoo", "Syrc"}
        if script in RTL_SCRIPTS and orientation != "rtl":
            entry["flags"] = ["orientation-mismatch"]

        # Flag duplicate symbols: the engine now tolerates them (first
        # definition wins, CAlphabetMap::Add) but the data should be fixed
        # upstream — 132 files carry them from the autoconversion era.
        symbol_counts = Counter(texts)
        dup_count = sum(1 for v in symbol_counts.values() if v > 1)
        if dup_count:
            entry.setdefault("flags", []).append("duplicate-symbols")
            entry["duplicate_symbols"] = dup_count

        alphabets.append(entry)

    alphabets.sort(key=lambda a: a["id"])

    # Dedupe by AlphID across corpus tiers. The engine's runtime map is
    # keyed by name with last-scan-wins; for the index we present one entry
    # per alphabet with an explicit tier priority and record the losing
    # variants so nothing is silently dropped.
    TIER_ORDER = {"maintained": 0, "worldalphabets": 1, "legacy": 2}
    by_id = {}
    for a in alphabets:
        cur = by_id.get(a["id"])
        if cur is None or TIER_ORDER[a["source"]] < TIER_ORDER[cur["source"]]:
            if cur is not None:
                a.setdefault("variants", []).extend(cur.get("variants", []))
                a["variants"].append(cur["file"])
            by_id[a["id"]] = a
        else:
            cur.setdefault("variants", []).append(a["file"])
    alphabets = sorted(by_id.values(), key=lambda a: a["id"])

    script_summary = Counter(a["script"] or "unknown" for a in alphabets)
    orientation_summary = Counter(a["orientation"] for a in alphabets)
    lang_coverage = sum(1 for a in alphabets if a["lang"])
    with_corpus = sum(1 for a in alphabets if a["training_available"])
    declaring = sum(1 for a in alphabets if a["training"])

    out = {
        "generator": "generate-alphabet-index.py",
        "generated_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "count": len(alphabets),
        "summaries": {
            "by_script": dict(sorted(script_summary.items(), key=lambda kv: -kv[1])),
            "by_orientation": dict(sorted(orientation_summary.items())),
            "with_lang_code": lang_coverage,
            "without_lang_code": len(alphabets) - lang_coverage,
            "declaring_training": declaring,
            "training_available": with_corpus,
            "training_missing": declaring - with_corpus,
            "no_training_declared": len(alphabets) - declaring,
        },
        "alphabets": alphabets,
    }
    if errors:
        out["parse_errors"] = errors

    OUT_FILE.write_text(json.dumps(out, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"wrote {OUT_FILE.relative_to(REPO)}: {len(alphabets)} alphabets, "
          f"{lang_coverage} with lang codes, {len(errors)} parse errors")
    return 0


if __name__ == "__main__":
    sys.exit(main())
