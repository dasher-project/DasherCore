#!/usr/bin/env python3
"""Cap training corpora at a byte budget.

Dasher's own guidance is "300K or more" of natural text — its character-level
PPM plateaus well before 400KB, and the trainer consumes the ENTIRE file at
alphabet-load time, so oversized corpora cost both shipping weight (74MB across
the corpus tail) and startup latency (a 3.8MB Bengali file is parsed line by
line every time that alphabet loads). Truncation keeps whole sentences,
preserving the licensing attribution chain (content is a prefix of the source).

Usage: python3 Scripts/cap-training-corpora.py [--max-bytes 409600] [--dry-run]
"""

import argparse
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
TRAINING = REPO / "Data" / "training"


def cap(text: str, max_bytes: int) -> str:
    out: list[str] = []
    total = 0
    for line in text.split("\n"):
        cost = len(line.encode("utf-8")) + 1
        if total + cost > max_bytes:
            break
        out.append(line)
        total += cost
    return "\n".join(out) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--max-bytes", type=int, default=400 * 1024)
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    saved = 0
    touched = 0
    for path in sorted(TRAINING.glob("training_*.txt")):
        original = path.read_bytes()
        if len(original) <= args.max_bytes:
            continue
        capped = cap(original.decode("utf-8"), args.max_bytes).encode("utf-8")
        saved += len(original) - len(capped)
        touched += 1
        if not args.dry_run:
            path.write_bytes(capped)
        print(f"  {path.name}: {len(original) // 1024}KB -> {len(capped) // 1024}KB")

    print(f"\n{'would cap' if args.dry_run else 'capped'} {touched} files, "
          f"{'saving' if not args.dry_run else 'would save'} {saved // 1024 // 1024}MB")
    return 0


if __name__ == "__main__":
    sys.exit(main())
