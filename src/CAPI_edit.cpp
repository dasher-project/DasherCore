// CAPI_edit.cpp — UTF-8 boundary math and edit-buffer anchoring: the RFC
// 0015 context-awareness helpers (todo.md Phase 2.7, moved verbatim from
// CAPI.cpp).
//
// Owns: the word/sentence/paragraph range walkers shared with the
// engine-side Interface overrides (capi::getRange), caret clamping and
// UTF-16/codepoint → byte-offset conversion for platform carets, and the
// exported set_offset / seed_buffer / offset-getter family.

#include "CAPI_internal.h"

#include <algorithm>
#include <cstring>
#include <string>

namespace {
size_t utf8CharLen(const std::string& s, size_t pos) {
    if (pos >= s.size()) {
        return 0;
    }
    const auto c = static_cast<unsigned char>(s[pos]);
    if (c < 0x80) {
        return 1;
    }
    if (c < 0xC0) {
        return 1; // continuation byte
    }
    if (c < 0xE0) {
        return 2;
    }
    if (c < 0xF0) {
        return 3;
    }
    return 4;
}

size_t nextChar(const std::string& s, size_t pos) {
    const size_t step = utf8CharLen(s, pos);
    return std::min(pos + (step > 0 ? step : 1), s.size());
}

size_t prevChar(const std::string& s, size_t pos) {
    if (pos == 0) {
        return 0;
    }
    size_t p = pos - 1;
    while (p > 0 && (static_cast<unsigned char>(s[p]) & 0xC0) == 0x80) {
        --p;
    }
    return p;
}

bool isSep(char c, const char* seps) {
    return std::strchr(seps, c) != nullptr;
}

// Forward search: skip non-separators, then skip separators
size_t findAfter(const std::string& s, size_t pos, const char* seps) {
    if (pos > s.size()) pos = s.size();
    size_t p = pos;
    while (p < s.size() && !isSep(s[p], seps)) {
        p = nextChar(s, p);
    }
    while (p < s.size() && isSep(s[p], seps)) {
        p = nextChar(s, p);
    }
    return p;
}

// Backward search: skip separators, then skip non-separators
size_t findBefore(const std::string& s, size_t pos, const char* seps) {
    if (pos >= s.size()) pos = s.size() > 0 ? s.size() - 1 : 0;
    size_t p = pos;
    while (p > 0 && isSep(s[p], seps)) {
        p = prevChar(s, p);
    }
    while (p > 0 && !isSep(s[p], seps)) {
        p = prevChar(s, p);
    }
    // if we landed on a separator, advance one
    if (p < s.size() && isSep(s[p], seps)) {
        p = nextChar(s, p);
    }
    return p;
}

constexpr const char* kWordSeps = " \t\v\f\r\n";
constexpr const char* kSentenceSeps = ".?!\r\n";
constexpr const char* kParagraphSeps = "\r\n";
} // namespace

// getRange is shared with the Interface overrides in CAPI.cpp —
// declared in CAPI_internal.h. Body verbatim from CAPI.cpp.
namespace capi {
void getRange(const std::string& buf, bool bForwards, Dasher::EditDistance dist, size_t& ioStart, size_t& ioEnd) {
    switch (dist) {
    case Dasher::EDIT_CHAR:
        if (bForwards) {
            ioEnd = std::min(nextChar(buf, ioEnd), buf.size());
        } else {
            ioStart = prevChar(buf, ioStart);
        }
        break;
    case Dasher::EDIT_WORD:
        if (bForwards) {
            ioEnd = findAfter(buf, ioEnd, kWordSeps);
        } else {
            ioStart = findBefore(buf, ioEnd > 0 ? ioEnd - 1 : 0, kWordSeps);
        }
        break;
    case Dasher::EDIT_SENTENCE:
        if (bForwards) {
            ioEnd = findAfter(buf, ioEnd, kSentenceSeps);
        } else {
            ioStart = findBefore(buf, ioEnd > 0 ? ioEnd - 1 : 0, kSentenceSeps);
        }
        break;
    case Dasher::EDIT_PARAGRAPH:
    case Dasher::EDIT_LINE:
        if (bForwards) {
            ioEnd = findAfter(buf, ioEnd, kParagraphSeps);
        } else {
            ioStart = findBefore(buf, ioEnd > 0 ? ioEnd - 1 : 0, kParagraphSeps);
        }
        break;
    case Dasher::EDIT_FILE:
    case Dasher::EDIT_ALL:
    case Dasher::EDIT_PAGE:
    case Dasher::EDIT_SELECTION:
    case Dasher::EDIT_NONE:
        if (bForwards) {
            ioEnd = buf.size();
        } else {
            ioStart = 0;
        }
        break;
    }
}
} // namespace capi

namespace {
// Realize on demand: seeding/re-anchoring can happen when a direct-entry
// frontend reads the target field before the first frame (e.g. on mode
// entry). Mirrors the deferred-Realize handling dasher_set_palette uses.
static bool ensure_realized_for_context(dasher_ctx* ctx) {
    if (ctx->realized) return true;
    if (!ctx->screen) return false;
    try {
        ctx->intf->Realize(nowMs());
        ctx->realized = true;
        return true;
    } catch (...) {
        return false;
    }
}

// Clamp a caret offset for anchoring: clamp to the buffer, then snap DOWN
// to the start of the containing codepoint. Offsets are UTF-8 bytes (the
// buffer's unit, matching GetContext/edit-output everywhere else in this
// CAPI), but platform carets arrive in character/UTF-16 units — a raw
// conversion can land mid-sequence, which would corrupt every subsequent
// edit. Snapping backward keeps the straddled codepoint in the pre-caret
// context rather than dropping it. Frontends convert units with
// dasher_byte_offset_from_utf16 / _from_codepoints; this is the guard that
// makes a sloppy conversion degrade instead of corrupt.
static int ValidatedSequenceLength(const unsigned char* p, int declared);
static size_t clamp_caret_to_codepoint(const std::string& buf, ptrdiff_t offset) {
    if (offset <= 0) return 0;
    const auto size = static_cast<ptrdiff_t>(buf.size());
    if (offset >= size) return buf.size();
    size_t pos = static_cast<size_t>(offset);
    int back = 0;
    while (pos > 0 && (static_cast<unsigned char>(buf[pos]) & 0xC0) == 0x80 && back < 3) {
        --pos;
        ++back;
    }
    if (back == 0) return pos; // already on a boundary
    // Landing on buf[pos]. If it's still a continuation (long stray run),
    // the original offset is a valid boundary (greptile: 4+ strays).
    if ((static_cast<unsigned char>(buf[pos]) & 0xC0) == 0x80) return static_cast<size_t>(offset);
    // The candidate lead byte at pos must form a VALID sequence with the
    // continuation bytes we walked over. In a malformed run (e.g. the
    // surrogate ED A0 80), the bytes look like lead + continuations but
    // are semantically stray — byte 1 is already a valid boundary under
    // the byte-per-byte contract, and snapping to 0 moves the anchor
    // BEFORE the wrong prefix (greptile: "malformed-byte boundary snaps
    // backward").
    const auto* p = reinterpret_cast<const unsigned char*>(buf.data()) + pos;
    unsigned char c = *p;
    int declared;
    if ((c & 0xF8) == 0xF0)
        declared = 4;
    else if ((c & 0xF0) == 0xE0)
        declared = 3;
    else if ((c & 0xE0) == 0xC0)
        declared = 2;
    else
        declared = 1;
    // If the lead isn't multi-byte, or declares fewer bytes than we walked
    // over, those "continuations" are strays — don't snap.
    if (declared <= 1 || declared < back + 1) return static_cast<size_t>(offset);
    // Semantically validate: overlong, surrogate, above-range all reject.
    if (ValidatedSequenceLength(p, declared) != declared) return static_cast<size_t>(offset);
    return pos; // valid sequence — snap to its start
}

// Shared UTF-8 walker for the unit-conversion helpers: decodes text forward,
// tracking byte position, codepoint count and UTF-16 unit count, stopping
// when the requested count is reached. count_utf16 selects the unit;
// target < 0 clamps to 0; a UTF-16 target landing mid-surrogate-pair
// resolves to the pair's start (the boundary BEFORE the second unit).
// Malformed sequences degrade byte-per-byte (each stray byte = one
// codepoint = one UTF-16 unit), so conversion never runs off the text.
// A lead byte's declared width is only real if the declared-1 following
// bytes are actual continuations (and not NUL). Otherwise this is a stray
// lead byte and degrades to ONE unit, so the walker re-examines the next
// byte as a fresh unit - byte-per-byte, as documented (greptile #83:
// "\xC2\x41" with offset 1 must be 1, not 2). The decoded VALUE is also
// validated: overlong encodings (2-byte < U+80, 3-byte < U+800, 4-byte
// < U+10000), UTF-16 surrogates (U+D800..U+DFFF), and values above
// U+10FFFF are malformed despite having continuation-shaped trailing
// bytes, and degrade the same way (greptile follow-up: "\xED\xA0\x80"
// with offset 1 must be 1, not 3).
static int ValidatedSequenceLength(const unsigned char* p, int declared) {
    for (int i = 1; i < declared; ++i) {
        if ((p[i] & 0xC0) != 0x80) return 1; // not a continuation (covers NUL)
    }
    // Decode the value for semantic validation.
    unsigned long cp;
    switch (declared) {
    case 2:
        cp = ((p[0] & 0x1FUL) << 6) | (p[1] & 0x3FUL);
        break;
    case 3:
        cp = ((p[0] & 0x0FUL) << 12) | ((p[1] & 0x3FUL) << 6) | (p[2] & 0x3FUL);
        break;
    case 4:
        cp = ((p[0] & 0x07UL) << 18) | ((p[1] & 0x3FUL) << 12) | ((p[2] & 0x3FUL) << 6) | (p[3] & 0x3FUL);
        break;
    default:
        return 1;
    }
    // Overlong: the value could have been encoded shorter.
    if (declared == 2 && cp < 0x80) return 1;
    if (declared == 3 && cp < 0x800) return 1;
    if (declared == 4 && cp < 0x10000) return 1;
    // Surrogates are not valid codepoints in UTF-8.
    if (cp >= 0xD800 && cp <= 0xDFFF) return 1;
    // Above the Unicode range.
    if (cp > 0x10FFFF) return 1;
    return declared;
}

static int byte_offset_from_count(const char* utf8_text, int target, bool count_utf16) {
    if (!utf8_text) return -1;
    if (target <= 0) return 0;
    long remaining = target;
    const unsigned char* p = reinterpret_cast<const unsigned char*>(utf8_text);
    int byte_pos = 0;
    while (*p) {
        unsigned char c = *p;
        // Lead-byte masks are mutually exclusive, so the check order is
        // free; grouping both one-unit cases (ASCII and stray
        // continuation/invalid) into the else keeps clang-tidy's
        // branch-clone check happy. Declared widths are validated — a
        // truncated or corrupt sequence degrades to one stray byte.
        int cp_bytes;
        if ((c & 0xF8) == 0xF0)
            cp_bytes = ValidatedSequenceLength(p, 4);
        else if ((c & 0xF0) == 0xE0)
            cp_bytes = ValidatedSequenceLength(p, 3);
        else if ((c & 0xE0) == 0xC0)
            cp_bytes = ValidatedSequenceLength(p, 2);
        else
            cp_bytes = 1; // ASCII (< 0x80) or stray continuation/invalid

        // UTF-16 units for this codepoint: 2 if it encodes >= U+10000.
        const int units = (count_utf16 && cp_bytes == 4) ? 2 : 1;
        if (remaining < units) {
            // UTF-16 target lands mid-surrogate-pair: resolve to the pair's
            // start (the boundary before this codepoint's second unit).
            break;
        }
        remaining -= units;
        // Skip the codepoint's bytes (or one stray byte). NB: guard against
        // running past NUL for truncated sequences — measure from the
        // sequence start, never from the advancing pointer (p[i] would
        // double-advance the lookahead and drop a byte).
        const unsigned char* seq = p;
        while (*p && (p - seq) < cp_bytes) {
            ++p;
            ++byte_pos;
        }
        if (remaining == 0) break;
    }
    return byte_pos;
}

} // namespace

extern "C" {

DASHER_API int dasher_get_offset(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf || !ctx->realized) return -1;
    auto* model = ctx->intf->GetModel();
    if (!model) return -1;
    return model->GetOffset();
}

DASHER_API int dasher_byte_offset_from_utf16(const char* utf8_text, int utf16_offset) {
    return byte_offset_from_count(utf8_text, utf16_offset, true);
}

DASHER_API int dasher_byte_offset_from_codepoints(const char* utf8_text, int codepoint_offset) {
    return byte_offset_from_count(utf8_text, codepoint_offset, false);
}

DASHER_API int dasher_set_offset(dasher_ctx* ctx, int offset) {
    if (!ctx || !ctx->intf) return -1;
    if (offset < 0) return -1;
    if (!ensure_realized_for_context(ctx)) return -1;
    return capi::guarded_result(ctx, "dasher_set_offset", -1, [&]() -> int {
        // Offsets are UTF-8 byte positions into the edit buffer (the CAPI's
        // universal unit); mid-sequence values from platform caret
        // conversions snap down to the codepoint start instead of
        // corrupting subsequent output (see clamp_caret_to_codepoint).
        const auto clamped = clamp_caret_to_codepoint(ctx->editBuffer, offset);
        ctx->cursorPos = clamped;
        ctx->intf->SetOffset(static_cast<unsigned int>(clamped), true);
        return 0;
    });
}

DASHER_API int dasher_seed_buffer(dasher_ctx* ctx, const char* text, int caret_offset) {
    if (!ctx || !ctx->intf) return -1;
    if (!ensure_realized_for_context(ctx)) return -1;
    return capi::guarded_result(ctx, "dasher_seed_buffer", -1, [&]() -> int {
        // Replace the buffer with the target field's text.
        ctx->editBuffer.assign(text ? text : "");
        // caret_offset is a UTF-8 byte position; platform caret units
        // (characters / UTF-16) must be converted by the frontend, and a
        // mid-codepoint value snaps down rather than corrupting output
        // (see clamp_caret_to_codepoint).
        const auto clamped = clamp_caret_to_codepoint(ctx->editBuffer, caret_offset);
        ctx->cursorPos = clamped;
        ctx->rateTimestamps.clear();

        // Subscribers must resync their mirrors WITHOUT injecting (RFC 0015:
        // backspacing a whole field into the target would destroy the user's
        // text — the same reasoning as Dasher-Windows #45's event-2 contract).
        notify_buffer_cleared(ctx);

        // Rebuild the model anchored at the caret: GetRoot seeds the LM from
        // GetContext (the CAPI Interface reads ctx->editBuffer), so
        // predictions continue from the text before the caret.
        ctx->intf->SetOffset(static_cast<unsigned int>(clamped), true);
        return 0;
    });
}

} // extern "C"
