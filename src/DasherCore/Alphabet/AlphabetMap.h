// AlphabetMap.h
//
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2002 Iain Murray
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef _WIN32
#include <sys/types.h>
#endif

#include "DasherCore/DasherTypes.h"

#include <vector>
#include <string>

namespace Dasher {
class CAlphabetMap;
}

/// \ingroup Alphabet
/// \{

/// Class used for fast conversion from training text (i.e. catenated
/// non-display text of symbols; Mandarin / Super-PinYin is a bit more
/// complicated but still uses one!) into Dasher's internal "symbol" indices.
/// One of these is created for the alphabet (CAlphInfo) currently in use,
/// tho there are no restrictions on creation of CAlphabetMaps in other places
/// (Mandarin!) - or modification, if you have a non-const pointer!
///
/// Ian clearly had reservations about this system, as follows; and I'd add
/// that much of the fun comes from supporting single unicode characters
/// which are multiple octets, as we use  std::string (which works in octets)
/// for everything. Since RFC 0020 the map also supports MULTI-unicode-
/// character symbols (digraph outputs, ZWJ emoji sequences, VS16 skin
/// tones) via longest-match probing in SymbolStream::next — see
/// LongestMatch(). Keys longer than the stream's 1024-byte window can
/// never match and are silently unregistered.
///
/// Note that in 2010 we did indeed tailor this to the alphabet more closely,
/// fast-casing single-octet characters to avoid using a hash etc. - which makes
/// many common alphabets substantially faster!
///
/// Anyway, Ian writes:
///
/// If I were just using GCC, which comes with the CGI "STL" implementation, I would
/// use hash_map (which isn't part of the ANSI/ISO standard C++ STL, but hey it's nice).
/// Using a plain map is just too slow for training on large files (or it is with certain
/// STL implementations). I'm sure training could be made much faster still, but that's
/// another matter...
///
/// While I could (and probably should) get a hash_map for VC++ from
/// http://www.stlport.org I thought it would be nicer if people didn't have
/// to download extra stuff and then have to get it working alongside the STL
/// with VC++, especially for just one small part of Dasher.
///
/// The result is this:
/// ***************************************************
/// very much thrown together to get Dasher out ASAP.
/// ***************************************************
/// It is deliberately not like an STL container.
/// However, as it has a tiny interface, it should still be easy to replace.
/// Sorry if this seems really unprofressional.
///
/// Replacing it might be a good idea. On the other hand it could be customised
/// to the needs of the alphabet, so that it works faster.
///
/// You can't remove items once they are added as Dasher has no need for that.
///
/// IAM 08/2002

#include "DasherCore/Messages.h"

class Dasher::CAlphabetMap {

  public:
    ~CAlphabetMap();

    /// Read-window size of SymbolStream: multi-codepoint keys of this
    /// length or longer can never be fully buffered for probing and are
    /// not registered (RFC 0020 — documented at Add).
    static constexpr size_t STREAM_WINDOW = 1024;

    // Return the symbol associated with Key or Undefined.
    symbol Get(const std::string& Key) const;
    symbol GetSingleChar(char key) const;

    /// Longest-match support (RFC 0020 clause 4): probe multi-codepoint
    /// keys (digraph outputs, ZWJ/VS16 emoji) at a buffer position, longest
    /// first, before the single-character path in SymbolStream::next().
    /// \param at buffer position; \param avail bytes readable from it
    /// \param matchedLen set to the matched key's byte length on success
    /// \return the symbol, or UNKNOWN_SYMBOL (0) when no key matches.
    symbol LongestMatch(const char* at, size_t avail, size_t& matchedLen) const;

    /// Longest multi-codepoint key in the map (0 when none) — the
    /// lookahead SymbolStream must keep buffered.
    size_t MaxKeyLen() const { return m_iMaxKeyLen; }

    class SymbolStream {
      public:
        virtual ~SymbolStream() = default;
        /// pMsgs used for reporting errors in utf8 encoding
        SymbolStream(std::istream& _in, CMessageDisplay* pMsgs = NULL);
        /// Gets the next symbol in the stream, using the specified AlphabetMap
        ///  to convert unicode characters to symbols.
        ///  \return 0 for unknown symbol (not in map); -1 for EOF; else symbol#.
        symbol next(const CAlphabetMap* map);

        /// Finds the next complete character in the stream,  but does not advance past it.
        ///  Hence, repeated calls will return the same string. (Always constructs a string,
        ///  which next() avoids for single-octet chars, so may be slower)
        ///  RFC 0020: when a multi-codepoint key starts at the current position,
        ///  returns the WHOLE key — exactly the bytes the next next() call would
        ///  consume — so annotation readers (Routing/Mandarin escape and route
        ///  parsing) never record a different token than they advance past.
        std::string peekAhead(const CAlphabetMap* map);

        /// Returns the string representation of the previous symbol (i.e. that returned
        ///  by the previous call to next()). Undefined if next() has not been called, or
        ///  if peekAhead() has been called since the last call to next(). Does not change
        ///  the stream position. Returns the full multi-codepoint key when the previous
        ///  symbol matched one (longest-match, RFC 0020) — not just its final codepoint.
        std::string peekBack();

        /// Bytes consumed by the last next() call — the source of
        /// peekBack's answer, kept as a copy because the read window can
        /// shift (ensureLookahead) between the two calls.
        std::string m_lastConsumed;

      protected:
        /// Called periodically to indicate some number of bytes have been read.
        ///  Default implementation does nothing; subclasses may override for e.g. logging.
        ///  \param num number of octets read _since_ the previous call.
        virtual void bytesRead(off_t num) {};

      private:
        /// Finds beginning of next unicode character, at position 'pos' or later,
        ///  filling buffer and skipping invalid characters as necessary.
        ///  Leaves 'pos' pointing at beginning of said character.
        ///  \return the number of octets representing the next character, or 0 for EOF
        ///  (inc. where the file ends with an incomplete character)
        inline int findNext();

        /// Ensure at least `want` bytes are buffered past pos (shifting the
        /// remaining window to the front and reading more; at EOF the buffer
        /// simply holds what's left). findNext's refill logic, parameterised.
        inline void ensureLookahead(size_t want);
        void readMore();
        char buf[STREAM_WINDOW];
        off_t pos, len;
        std::istream& in;
        CMessageDisplay* const m_pMsgs;
        /// Count of invalid UTF-8 bytes skipped so far (aggregated — see
        /// findNext); flushed as a single summary message on every EOF path.
        int m_skippedInvalid = 0;
    };

    // Fills Symbols with the symbols corresponding to Input. {{{ Note that this
    // is not necessarily reversible by repeated use of GetText. Some text
    // may not be recognised; any such will be turned into symbol number 0.}}}
    void GetSymbols(std::vector<symbol>& Symbols, const std::string& Input) const;

    CAlphabetMap(unsigned int InitialTableSize = 255);
    void AddParagraphSymbol(symbol Value);

    /// Add a symbol to the map
    ///  \param Key text of the symbol; must not be present already
    ///  \param Value symbol number to which that text should be mapped
    void Add(const std::string& Key, symbol Value);

  private:
    class Entry {
      public:
        Entry(std::string Key, symbol Symbol, Entry* Next) : Key(Key), Symbol(Symbol), Next(Next) {}
        std::string Key;
        symbol Symbol;
        Entry* Next;
    };

    // A standard hash -- could try and research something specific.
    inline unsigned int Hash(const std::string& Input) const {
        unsigned int Result = 0;

        typedef std::string::const_iterator CI;
        CI Cur = Input.begin();
        CI end = Input.end();

        while (Cur != end)
            Result = (Result << 1) ^ *Cur++;
        Result %= HashTable.size();

        return Result;
        /*
           if (Input.size()==1) // Speedup for ASCII text
           return Input[0];

           for (int i=0; i<Input.size(); i++)
           Result = (Result<<1)^Input[i];

           return Result%HashTable.size();
         */
    }
    std::vector<Entry> Entries;
    std::vector<Entry*> HashTable;
    symbol* m_pSingleChars;
    /// both "\r\n" and "\n" are mapped to this (if not Undefined).
    /// (Historically the only multi-character mapping; multi-codepoint
    /// keys via Add() now exist too — see LongestMatch.)
    symbol m_ParagraphSymbol;

    /// Multi-codepoint keys (copies, with their symbols), sorted longest
    /// first — copies because Entries vector growth relocates its strings.
    /// Only keys that the single-character path can never match (more than
    /// one codepoint) belong here.
    std::vector<std::pair<std::string, symbol>> m_vMultiCharKeys;
    size_t m_iMaxKeyLen = 0;
};
/// \}
