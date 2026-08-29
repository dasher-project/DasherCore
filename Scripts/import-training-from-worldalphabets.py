#!/usr/bin/env python3
"""Import Dasher training corpora from a WorldAlphabets checkout.

The licensing-clean source for training text (DasherCore issue #70
follow-up): WorldAlphabets builds generic per-language text corpora
(scripts/build_text_corpora.py -> data/corpora/<lang>.txt) under its own
licensing control — the GPL upstream Dasher repo is NOT a usable source
(GPL->MIT relicensing not agreed; see closed PR #72).

This script maps that generic asset onto DasherCore's conventions:

  data/corpora/<lang>.txt  ->  Data/training/training_wa_<lang>_<Script>.txt

- Driven by the generated alphabet index: only languages whose alphabets
  declare a missing corpus are imported, and each corpus is named for the
  script its alphabets actually use.
- Never overwrites corpora DasherCore already ships (real sentences beat
  synthetic text).
- Only imports corpora whose SOURCES.json entry has verify != true, unless
  --allow-unverified is given (the review gate lives in WorldAlphabets).
- Optionally retargets the legacy trainingFilename declarations of
  corpus-less alphabets to the imported wa names (--retarget).

Usage:
  python3 Scripts/import-training-from-worldalphabets.py \
      --worldalphabets /path/to/WorldAlphabets [--retarget] [--dry-run]
"""

import argparse
import json
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
INDEX = REPO / "Data" / "alphabets" / "alphabet_index.json"
TRAINING = REPO / "Data" / "training"
ALPHABETS = REPO / "Data" / "alphabets"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--worldalphabets", required=True, help="path to a WorldAlphabets checkout")
    ap.add_argument("--retarget", action="store_true",
                    help="rewrite legacy trainingFilename declarations to imported wa corpora")
    ap.add_argument("--allow-unverified", action="store_true",
                    help="import corpora still flagged verify:true in SOURCES.json")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    wa = Path(args.worldalphabets)
    corpora = wa / "data" / "corpora"
    sources = json.loads((corpora / "SOURCES.json").read_text(encoding="utf-8")) if (corpora / "SOURCES.json").exists() else {}

    index = json.loads(INDEX.read_text(encoding="utf-8"))
    have = {p.name for p in TRAINING.glob("training_*")}

    # (lang, script) pairs whose alphabets declare corpora we don't ship
    needed: set[tuple[str, str]] = set()
    for a in index["alphabets"]:
        if not a.get("training") or a["training"] in have or a["training"] == "":
            continue
        if a.get("lang") and a.get("script"):
            needed.add((a["lang"].split("-")[0], a["script"]))

    imported, skipped_verify, skipped_missing = [], [], []
    for lang, script in sorted(needed):
        name = f"training_wa_{lang}_{script}.txt"
        if name in have:
            continue
        src = corpora / f"{lang}.txt"
        if not src.exists():
            skipped_missing.append(f"{lang}_{script}")
            continue
        meta = sources.get(f"{lang}.txt", {})
        if meta.get("verify") and not args.allow_unverified:
            skipped_verify.append(f"{lang}_{script}")
            continue
        if not args.dry_run:
            (TRAINING / name).write_bytes(src.read_bytes())
        imported.append(name)

    print(f"{'would import' if args.dry_run else 'imported'} {len(imported)} corpora")
    for n in imported[:10]:
        print("  ", n)
    if len(imported) > 10:
        print(f"   ... and {len(imported) - 10} more")
    if skipped_verify:
        print(f"skipped (verify:true in SOURCES.json — review in WorldAlphabets): {' '.join(skipped_verify)}")
    if skipped_missing:
        print(f"skipped (no corpus built in WorldAlphabets): {' '.join(skipped_missing)}")

    if args.retarget and imported:
        done = 0
        # Key by (lang, script) AND by lang — lookup prefers an exact
        # script match (a Bopomofo alphabet must train on the Bopo corpus,
        # not whatever Latin one was imported last for the same language)
        # and falls back to lang-only when the alphabet's script has no
        # corpus of its own.
        by_lang: dict[str, list[tuple[str, str]]] = {}
        for name in imported:
            lang, script = name[len("training_wa_"):-len(".txt")].rsplit("_", 1)
            by_lang.setdefault(lang, []).append((script, name))

        def corpus_for(lang: str, script: str | None) -> str | None:
            cands = by_lang.get(lang)
            if not cands:
                return None
            if script:
                exact = next((f for s, f in cands if s == script), None)
                if exact:
                    return exact
            return cands[0][1]

        for path in list(ALPHABETS.rglob("alphabet*.xml")):
            text = path.read_text(encoding="utf-8")
            changed = False
            for a in index["alphabets"]:
                old = a.get("training") or ""
                if not old or old.startswith("training_wa_"):
                    continue
                lang = (a.get("lang") or "").split("-")[0]
                new = corpus_for(lang, a.get("script"))
                if new and old in text:
                    text = text.replace(old, new)
                    changed = True
            if changed:
                if not args.dry_run:
                    path.write_text(text, encoding="utf-8")
                done += 1
        print(f"{'would retarget' if args.dry_run else 'retargeted'} legacy declarations in {done} alphabet files")

    # Undeclared alphabets: WA exports never set trainingFilename, so the
    # engine trains them uniform forever. Where an imported corpus matches
    # the alphabet's language (exact script preferred), inject the
    # declaration — the single biggest coverage win for the WA corpus tail.
    # Runs on every --retarget invocation (idempotent: skips files that
    # already declare a corpus), not only when new corpora arrived.
    if args.retarget:
        by_lang: dict[str, list[tuple[str, str]]] = {}
        # Every wa corpus on disk (not just this run's imports — idempotent
        # re-runs must still inject into newly-noticed undeclared alphabets).
        for path in sorted(TRAINING.glob("training_wa_*.txt")):
            lang, script = path.name[len("training_wa_"):-len(".txt")].rsplit("_", 1)
            by_lang.setdefault(lang, []).append((script, path.name))
        injected = 0
        for a in index["alphabets"]:
            if a.get("training"):
                continue
            lang = (a.get("lang") or "").split("-")[0]
            cands = by_lang.get(lang)
            if not cands:
                continue
            script = a.get("script") or ""
            corpus = next((f for s, f in cands if s == script), cands[0][1])
            path = ALPHABETS / a["file"]
            text = path.read_text(encoding="utf-8")
            if 'trainingFilename=""' in text:
                # WA exports emit an EMPTY attribute — fill it
                text = text.replace('trainingFilename=""', f'trainingFilename="{corpus}"', 1)
            elif "trainingFilename" not in text:
                text = text.replace("<alphabet ", f'<alphabet trainingFilename="{corpus}" ', 1)
            else:
                continue  # already declares something real
            if not args.dry_run:
                path.write_text(text, encoding="utf-8")
            injected += 1
        print(f"{'would inject' if args.dry_run else 'injected'} trainingFilename into {injected} undeclared alphabets")

    if imported and not args.dry_run:
        print("\nNext: regenerate the index (Scripts/generate-alphabet-index.py) and commit")
    return 0


if __name__ == "__main__":
    sys.exit(main())
