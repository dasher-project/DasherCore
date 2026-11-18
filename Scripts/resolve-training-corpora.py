#!/usr/bin/env python3
"""Materialize training corpora for alphabets whose declared file is missing (#119).

Several shipped alphabet definitions name training files that were never added
to Data/training, although the engine ships a compatible corpus for the same
language and script (the training_wa_<lang>_<script>.txt family). Until now
only the Apple frontend papered over this by writing alias files at app
packaging time; every other frontend started those alphabets untrained.

This script resolves the gap in the shared data so every frontend benefits:

  For each alphabet whose trainingFilename does not exist in Data/training,
  IF its index entry has both a lang and a script, AND its conversion is
  "none" (Mandarin/pinyin conversion alphabets are excluded — their training
  files have conversion-specific formats), AND Data/training contains the
  canonical corpus training_wa_<lang>_<script>.txt for exactly that
  (lang, script) pair, THEN a copy of that corpus is written under the
  alphabet's declared (legacy) filename.

Deliberately conservative — this cannot pick "the wrong" corpus:

  - The mapping key is the exact (lang, script) pair; the target filename is
    derived from that pair alone, never from fuzzy name similarity, so e.g.
    the ja+Kana-tagged "Japanese Canna" alphabet resolves only to
    training_wa_ja_Kana.txt (which ships byte-identical to the ja_Hira and
    ja_Jpan corpora, so no wrong-script distribution is possible), while the
    "Pure Katakana" alphabets — whose lang metadata is corrupt (ak, Akan) —
    resolve to nothing and stay untrained.
  - Alphabets with corrupted/absent lang metadata (e.g. the "Pure Katakana"
    definitions tagged lang=ak) have no canonical corpus and stay untrained.
  - Existing files are never overwritten: an alphabet whose declared training
    file already ships (or is later added for real) keeps it untouched, so
    deliberate alphabet/corpus pairings and user training files collected
    under legacy names keep their scope — the engine's exact-filename lookup
    order is unchanged.
  - Combining-accents alphabets (id contains "combining accents") receive an
    NFD-normalized copy: their symbols are decomposed sequences, and the wa_
    corpora are NFC (same treatment as the Apple workaround this replaces).

The script also flips training_available in Data/alphabets/alphabet_index.json
to the value generate-alphabet-index.py would compute once the files exist,
and registers new files in Data/training/Makefile.am.

Idempotent: run repeatedly, or with --dry-run to review the plan.
"""

import argparse
import json
import unicodedata
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TRAINING = ROOT / "Data" / "training"
INDEX = ROOT / "Data" / "alphabets" / "alphabet_index.json"
MAKEFILE_AM = TRAINING / "Makefile.am"

WA_PREFIX = "training_wa_"


def canonical_corpora():
    """Map (lang, script) -> shipped canonical wa_ corpus filename."""
    canon = {}
    for path in sorted(TRAINING.glob(WA_PREFIX + "*.txt")):
        stem = path.name[: -len(".txt")]
        try:
            _, lang, script = stem.rsplit("_", 2)
        except ValueError:
            continue
        # The filename encodes the pair, so at most one file per key exists;
        # a duplicate would be a data bug worth failing on.
        if (lang, script) in canon:
            raise SystemExit(f"ambiguous canonical corpus for ({lang}, {script})")
        canon[(lang, script)] = path.name
    return canon


def plan(index, canon):
    """Yield (alphabet, declared_name, source_path, wants_nfd) for every alias to create."""
    for alpha in index["alphabets"]:
        declared = alpha.get("training")
        if not declared or (TRAINING / declared).exists():
            continue
        if alpha.get("conversion") != "none":
            continue
        lang, script = alpha.get("lang"), alpha.get("script")
        if not lang or not script:
            continue
        source = canon.get((lang, script))
        if not source:
            continue  # no compatible corpus ships: stays untrained, by design
        wants_nfd = "combining accents" in alpha["id"].lower()
        yield alpha, declared, TRAINING / source, wants_nfd


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--dry-run", action="store_true", help="print the plan without writing")
    args = ap.parse_args()

    index = json.loads(INDEX.read_text(encoding="utf-8"))
    canon = canonical_corpora()

    written, skipped = [], []
    seen_targets = {}
    for alpha, declared, source, wants_nfd in plan(index, canon):
        target = TRAINING / declared
        if declared in seen_targets:
            # Two alphabets sharing one legacy name must agree on the source
            # and the normalization form, else the data is inconsistent.
            if seen_targets[declared] != (source, wants_nfd):
                raise SystemExit(
                    f"conflicting sources for {declared}: {seen_targets[declared]} vs {(source, wants_nfd)}"
                )
            continue
        seen_targets[declared] = (source, wants_nfd)
        form = "NFD" if wants_nfd else "NFC"
        text = unicodedata.normalize(form, source.read_text(encoding="utf-8"))
        if not unicodedata.is_normalized(form, text):
            raise SystemExit(f"{source}: normalization to {form} failed")
        if args.dry_run:
            print(f"{form}  {declared}  <-  {source.name}  ({alpha['id']})")
            continue
        if target.exists():  # never clobber: a real corpus landed for this name
            skipped.append(declared)
            continue
        target.write_text(text, encoding="utf-8")
        written.append((declared, form))

    if args.dry_run:
        return

    # Register new files in the autotools dist list, preserving the existing
    # line order and inserting the new entries in sorted position.
    mk_lines = MAKEFILE_AM.read_text(encoding="utf-8").splitlines()
    data_idx = [i for i, l in enumerate(mk_lines) if l.startswith("\t") and ".txt" in l]
    listed = {mk_lines[i].strip("\t \\").strip() for i in data_idx}
    missing = sorted(p.name for p in TRAINING.glob("*.txt") if p.name not in listed)
    if missing:
        for name in missing:
            insert_at = next(
                (i for i in data_idx if mk_lines[i].strip("\t \\").strip() > name), data_idx[-1] + 1
            )
            if insert_at > data_idx[-1]:
                # Appending after the current final entry: it was terminal
                # (no continuation backslash) — extend it and make the new
                # line terminal instead.
                last = data_idx[-1]
                if not mk_lines[last].rstrip().endswith("\\"):
                    mk_lines[last] = mk_lines[last].rstrip() + " \\"
                mk_lines.insert(insert_at, f"\t{name}\t\t")
            else:
                mk_lines.insert(insert_at, f"\t{name}\t\t \\")
            data_idx = [i for i, l in enumerate(mk_lines) if l.startswith("\t") and ".txt" in l]
        # Sanity: every data line except the last must continue; the last
        # must not (a dangling continuation is a make syntax error).
        for i in data_idx[:-1]:
            assert mk_lines[i].rstrip().endswith("\\"), f"Makefile.am line {i + 1} lost its continuation"
        assert not mk_lines[data_idx[-1]].rstrip().endswith("\\"), "Makefile.am ends with a dangling continuation"
        MAKEFILE_AM.write_text("\n".join(mk_lines) + "\n", encoding="utf-8")

    # Flip training_available to what the generator would now compute, and
    # keep the summaries block in step (it counts the same flags).
    flipped = 0
    for alpha in index["alphabets"]:
        declared = alpha.get("training")
        available = bool(declared) and (TRAINING / declared).exists()
        if alpha.get("training_available") != available:
            alpha["training_available"] = available
            flipped += 1
    summaries = index.get("summaries", {})
    if "training_available" in summaries:
        summaries["training_available"] = sum(
            1 for a in index["alphabets"] if a.get("training_available")
        )
    if "training_missing" in summaries:
        summaries["training_missing"] = sum(
            1 for a in index["alphabets"] if a.get("training") and not a.get("training_available")
        )
    INDEX.write_text(json.dumps(index, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")

    print(f"aliases written: {len(written)} ({sum(1 for _, f in written if f == 'NFD')} NFD)")
    print(f"existing files kept: {len(skipped)}")
    print(f"index entries updated: {flipped}")
    if missing:
        print(f"Makefile.am: {len(missing)} entr{'y' if len(missing) == 1 else 'ies'} added")


if __name__ == "__main__":
    main()
