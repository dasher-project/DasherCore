// Longest-match symbol lookup (RFC 0020 clause 4).
//
// SymbolStream::next historically read exactly ONE unicode character per
// lookup, so multi-codepoint symbols (digraph outputs, ZWJ emoji sequences,
// VS16 skin tones) could never be matched from training text — the trainer
// split them into per-codepoint unknowns. These tests pin the new
// longest-match-first behaviour at the CAlphabetMap level.
#include "test_common.h"

#include "DasherCore/Alphabet/AlphabetMap.h"

#include <sstream>
#include <string>

using Dasher::CAlphabetMap;

namespace {
// 👨‍👩‍👧 = U+1F468 ZWJ U+1F469 ZWJ U+1F467 (18 bytes)
const std::string FAMILY = "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA7";
// ❤️ = U+2764 U+FE0F (6 bytes); ❤ = U+2764 (3 bytes)
const std::string HEART_VS = "\xE2\x9D\xA4\xEF\xB8\x8F";
const std::string HEART = "\xE2\x9D\xA4";
} // namespace

TEST(map_longest_match_multi_codepoint_symbol) {
    CAlphabetMap map;
    map.Add("a", 2);
    map.Add(FAMILY, 5);

    std::istringstream in("a" + FAMILY + "a");
    CAlphabetMap::SymbolStream syms(in);
    ASSERT_EQ(syms.next(&map), 2);
    ASSERT_EQ(syms.next(&map), 5); // the whole ZWJ sequence, one symbol
    ASSERT_EQ(syms.next(&map), 2);
    ASSERT_EQ(syms.next(&map), -1); // EOF
}

TEST(map_longest_match_prefers_multi_codepoint_over_prefix) {
    // ❤️ must win over its first-codepoint prefix ❤ even though ❤ is also
    // a symbol — this is the ordering bug that made VS16 carriers
    // untrainable while their bare hearts trained fine.
    CAlphabetMap map;
    map.Add(HEART, 3);
    map.Add(HEART_VS, 4);

    std::istringstream in(HEART_VS + HEART);
    CAlphabetMap::SymbolStream syms(in);
    ASSERT_EQ(syms.next(&map), 4);
    ASSERT_EQ(syms.next(&map), 3);
    ASSERT_EQ(syms.next(&map), -1);
}

TEST(map_longest_match_ascii_digraph) {
    // Digraph outputs (lam-alef ligatures etc.) become trainable text too.
    CAlphabetMap map;
    map.Add("a", 2);
    map.Add("b", 3);
    map.Add("ab", 7);

    std::istringstream in("ab b a");
    CAlphabetMap::SymbolStream syms(in);
    ASSERT_EQ(syms.next(&map), 7);
    ASSERT_EQ(syms.next(&map), 0); // ' ' unknown in this map
    ASSERT_EQ(syms.next(&map), 3); // lone b — not the digraph
    ASSERT_EQ(syms.next(&map), 0);
    ASSERT_EQ(syms.next(&map), 2);
    ASSERT_EQ(syms.next(&map), -1);
}

TEST(map_longest_match_across_buffer_refill) {
    // The 1024-byte stream buffer refills by shifting the remaining window
    // to the front; a multi-codepoint key straddling the refill boundary
    // must still match (ensureLookahead keeps MaxKeyLen bytes available).
    CAlphabetMap map;
    map.Add("a", 2);
    map.Add(FAMILY, 5);

    std::string padded(1020, 'a');
    std::istringstream in(padded + FAMILY + "a");
    CAlphabetMap::SymbolStream syms(in);
    for (int i = 0; i < 1020; i++)
        ASSERT_EQ(syms.next(&map), 2);
    ASSERT_EQ(syms.next(&map), 5); // straddles the 1024-byte boundary
    ASSERT_EQ(syms.next(&map), 2);
    ASSERT_EQ(syms.next(&map), -1);
}

TEST(map_longest_match_absent_falls_back_cleanly) {
    // Map with only single-codepoint keys: the probe list is empty and
    // behaviour is byte-identical to the pre-RFC stream.
    CAlphabetMap map;
    map.Add("a", 2);
    map.Add(HEART, 3);

    std::istringstream in(HEART + "a");
    CAlphabetMap::SymbolStream syms(in);
    ASSERT_EQ(syms.next(&map), 3);
    ASSERT_EQ(syms.next(&map), 2);
    ASSERT_EQ(syms.next(&map), -1);
}

TEST(map_longest_match_unknown_text_stays_unknown) {
    // Multi-codepoint text with no matching key still degrades to
    // per-codepoint lookups (unknowns), never a bogus match.
    CAlphabetMap map;
    map.Add("a", 2);
    map.Add(FAMILY, 5);

    std::istringstream in("\xF0\x9F\x91\xA8"
                          "a"); // lone 👨, not in the map
    CAlphabetMap::SymbolStream syms(in);
    ASSERT_EQ(syms.next(&map), 0); // 👨 unknown
    ASSERT_EQ(syms.next(&map), 2);
    ASSERT_EQ(syms.next(&map), -1);
}

TEST(map_longest_match_eof_mid_key_degrades_cleanly) {
    // File ending with a TRUNCATED multi-codepoint key: the probe can't
    // match (avail < key length), and the per-codepoint path reports the
    // lead codepoint as unknown — no crash, no bogus symbol.
    CAlphabetMap map;
    map.Add("a", 2);
    map.Add(FAMILY, 5);

    std::istringstream in("a" + FAMILY.substr(0, 7)); // family cut mid-sequence
    CAlphabetMap::SymbolStream syms(in);
    ASSERT_EQ(syms.next(&map), 2);
    Dasher::symbol s;
    while ((s = syms.next(&map)) != -1)
        ASSERT_EQ(s, 0); // fragments unknown
}

TEST(map_longest_match_duplicate_add_registers_once) {
    // Add tolerates duplicates (first wins) — the multi-codepoint
    // registration must not double up either.
    CAlphabetMap map;
    map.Add(FAMILY, 5);
    map.Add(FAMILY, 9); // duplicate key, ignored

    std::istringstream in(FAMILY);
    CAlphabetMap::SymbolStream syms(in);
    ASSERT_EQ(syms.next(&map), 5); // first registration wins
    ASSERT_EQ(syms.next(&map), -1);
}

TEST(map_peek_back_returns_whole_matched_key) {
    // peekBack's contract ("string representation of the previous symbol")
    // must survive longest-match: the buffer walk it replaced would have
    // returned only the FINAL codepoint of a multi-codepoint key.
    CAlphabetMap map;
    map.Add("a", 2);
    map.Add(FAMILY, 5);

    std::istringstream in("a" + FAMILY);
    CAlphabetMap::SymbolStream syms(in);
    ASSERT_EQ(syms.next(&map), 2);
    ASSERT(syms.peekBack() == "a");
    ASSERT_EQ(syms.next(&map), 5);
    ASSERT(syms.peekBack() == FAMILY); // the whole 18-byte key, not 👧
    // peekBack does not advance the stream
    ASSERT_EQ(syms.next(&map), -1);
}

TEST(map_peek_ahead_agrees_with_next) {
    // Greptile P1: annotation readers (Routing/Mandarin conversion
    // trainers, CTrainer::readEscape) record the peeked token and then
    // advance via next(). peekAhead must therefore return EXACTLY the
    // bytes the following next() consumes — including a whole
    // multi-codepoint key, not just its first codepoint.
    CAlphabetMap map;
    map.Add("a", 2);
    map.Add(FAMILY, 5);

    std::istringstream in(FAMILY + "a");
    CAlphabetMap::SymbolStream syms(in);
    ASSERT(syms.peekAhead(&map) == FAMILY);
    ASSERT_EQ(syms.next(&map), 5);
    ASSERT(syms.peekAhead(&map) == "a");
    ASSERT_EQ(syms.next(&map), 2);
    ASSERT_EQ(syms.next(&map), -1);
}

TEST(map_raw_mode_protects_delimiters_from_shadowing) {
    // Greptile P1 (round 3): if a structural delimiter (annotation stop
    // '>' etc.) is the PREFIX of a multi-codepoint key, longest-match
    // would swallow it into the key — the annotation loop then misses its
    // terminator and absorbs the rest of the training file. nextRaw /
    // peekAheadRaw give annotation/escape readers one codepoint at a time,
    // exactly the pre-RFC behaviour, so delimiters always terminate.
    CAlphabetMap map;
    map.Add(">", 3);  // the stop delimiter IS a symbol (typical)
    map.Add(">x", 7); // multi-codepoint key sharing the prefix
    map.Add("b", 2);

    // Raw (structural) mode: '>' terminates one codepoint at a time.
    std::istringstream in1(">b>x");
    CAlphabetMap::SymbolStream raw(in1);
    ASSERT_EQ(raw.nextRaw(&map), 3);
    ASSERT_EQ(raw.nextRaw(&map), 2);
    ASSERT(raw.peekAheadRaw() == ">");
    ASSERT_EQ(raw.nextRaw(&map), 3); // delimiter consumed ALONE, not as ">x"
    ASSERT_EQ(raw.nextRaw(&map), 0); // the trailing 'x' is now unknown

    // Training mode over the same bytes: longest-match wins — ">x" is one
    // symbol, the delimiter shadowed. That is the intended behaviour for
    // symbol streams and precisely why structural readers use the raw form.
    std::istringstream in2(">x");
    CAlphabetMap::SymbolStream lm(in2);
    ASSERT_EQ(lm.next(&map), 7);
    ASSERT_EQ(lm.next(&map), -1);
}
