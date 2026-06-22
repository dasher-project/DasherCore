// UTF conversion tests: verify UTF-8/16/32 round-trips using DasherCore's ConvertUTF
#include "test_common.h"
#include "DasherCore/Common/Unicode/ConvertUTF.h"

TEST(utf8_ascii_roundtrip) {
    UTF8 source[] = "Hello";
    UTF8* srcStart = source;
    UTF8* srcEnd = source + 5;
    UTF16 target[16];
    UTF16* tgtStart = target;
    UTF16* tgtEnd = target + 16;

    ConversionResult cr = ConvertUTF8toUTF16((const UTF8**)&srcStart, srcEnd, &tgtStart, tgtEnd, lenientConversion);
    ASSERT_EQ(cr, conversionOK);

    int utf16_len = tgtStart - target;
    printf("  'Hello' -> %d UTF16 units\n", utf16_len);
    ASSERT_EQ(utf16_len, 5);

    UTF8 back[16];
    UTF8* backStart = back;
    UTF8* backEnd = back + 16;
    const UTF16* src16 = target;
    cr = ConvertUTF16toUTF8(&src16, target + utf16_len, &backStart, backEnd, lenientConversion);
    ASSERT_EQ(cr, conversionOK);
    *backStart = 0;
    ASSERT_STR_EQ((char*)back, "Hello");

}

TEST(utf8_multibyte_roundtrip) {
    // "héllo" - é is U+00E9 (2 bytes in UTF-8, 1 unit in UTF-16)
    const char* utf8_str = "h\xc3\xa9llo";
    int utf8_len = strlen(utf8_str);

    UTF8* srcStart = (UTF8*)utf8_str;
    UTF8* srcEnd = (UTF8*)utf8_str + utf8_len;
    UTF16 target[32];
    UTF16* tgtStart = target;
    UTF16* tgtEnd = target + 32;

    ConversionResult cr = ConvertUTF8toUTF16((const UTF8**)&srcStart, srcEnd, &tgtStart, tgtEnd, lenientConversion);
    ASSERT_EQ(cr, conversionOK);
    int utf16_len = tgtStart - target;
    printf("  'héllo' (%d UTF8 bytes) -> %d UTF16 units\n", utf8_len, utf16_len);
    ASSERT_EQ(utf16_len, 5);

    UTF8 back[32];
    UTF8* backStart = back;
    UTF8* backEnd = back + 32;
    const UTF16* src16 = target;
    cr = ConvertUTF16toUTF8(&src16, target + utf16_len, &backStart, backEnd, lenientConversion);
    ASSERT_EQ(cr, conversionOK);
    *backStart = 0;
    ASSERT_STR_EQ((char*)back, utf8_str);

}

TEST(utf8_emoji_roundtrip) {
    // "a😀b" - 😀 is U+1F600 (4 bytes UTF-8, 2 units UTF-16 surrogate pair)
    const char* utf8_str = "a\xf0\x9f\x98\x80"
                           "b";
    int utf8_len = strlen(utf8_str);

    UTF8* srcStart = (UTF8*)utf8_str;
    UTF8* srcEnd = (UTF8*)utf8_str + utf8_len;
    UTF16 target[32];
    UTF16* tgtStart = target;
    UTF16* tgtEnd = target + 32;

    ConversionResult cr = ConvertUTF8toUTF16((const UTF8**)&srcStart, srcEnd, &tgtStart, tgtEnd, lenientConversion);
    ASSERT_EQ(cr, conversionOK);
    int utf16_len = tgtStart - target;
    printf("  'a😀b' (%d UTF8 bytes) -> %d UTF16 units\n", utf8_len, utf16_len);
    ASSERT_EQ(utf16_len, 4); // a + surrogate pair + b

    UTF8 back[32];
    UTF8* backStart = back;
    UTF8* backEnd = back + 32;
    const UTF16* src16 = target;
    cr = ConvertUTF16toUTF8(&src16, target + utf16_len, &backStart, backEnd, lenientConversion);
    ASSERT_EQ(cr, conversionOK);
    *backStart = 0;
    ASSERT_STR_EQ((char*)back, utf8_str);

}

TEST(utf8_utf32_roundtrip) {
    const char* utf8_str = "Hello\xE4\xB8\x96\xE7\x95\x8C"; // "Hello世界"
    int utf8_len = strlen(utf8_str);

    UTF8* srcStart = (UTF8*)utf8_str;
    UTF8* srcEnd = (UTF8*)utf8_str + utf8_len;
    UTF32 target[32];
    UTF32* tgtStart = target;
    UTF32* tgtEnd = target + 32;

    ConversionResult cr = ConvertUTF8toUTF32((const UTF8**)&srcStart, srcEnd, &tgtStart, tgtEnd, lenientConversion);
    ASSERT_EQ(cr, conversionOK);
    int utf32_len = tgtStart - target;
    printf("  'Hello世界' -> %d UTF32 units\n", utf32_len);
    ASSERT_EQ(utf32_len, 7); // 5 ASCII + 2 CJK

    UTF8 back[64];
    UTF8* backStart = back;
    UTF8* backEnd = back + 64;
    const UTF32* src32 = target;
    cr = ConvertUTF32toUTF8(&src32, target + utf32_len, &backStart, backEnd, lenientConversion);
    ASSERT_EQ(cr, conversionOK);
    *backStart = 0;
    ASSERT_STR_EQ((char*)back, utf8_str);

}

TEST(utf8_empty_string) {
    UTF8 source[] = "";
    UTF8* srcStart = source;
    UTF8* srcEnd = source;
    UTF16 target[8];
    UTF16* tgtStart = target;
    UTF16* tgtEnd = target + 8;

    ConversionResult cr = ConvertUTF8toUTF16((const UTF8**)&srcStart, srcEnd, &tgtStart, tgtEnd, lenientConversion);
    ASSERT_EQ(cr, conversionOK);
    ASSERT_EQ(tgtStart - target, 0);

}

TEST(utf8_legal_sequence_check) {
    UTF8 valid[] = "Hello";
    ASSERT(isLegalUTF8Sequence(valid, valid + 5));

    UTF8 single[1] = {'A'};
    ASSERT(isLegalUTF8Sequence(single, single + 1));

}

TEST(utf8_replacement_char) {
    ASSERT_EQ(UNI_REPLACEMENT_CHAR, (UTF32)0x0000FFFD);
}
