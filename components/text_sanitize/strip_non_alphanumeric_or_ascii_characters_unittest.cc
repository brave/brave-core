/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "brave/components/text_sanitize/text_sanitize_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace text_sanitize {

TEST(StripNonAlphanumericAsciiCharactersTest,
     StripNonAlphanumericAsciiCharacters) {
  const std::vector<std::tuple<std::string, std::string>> samples = {
      {R"(<meta property="og:title" content="test">)",
       R"(<meta property="og:title" content="test">)"},
      {" The quick brown fox jumps over the lazy dog. "
       "$123,000.0 !\"#$%&'()*+,-./:<=>?@\\[]^_`{|}~ 0123456789 \t\n\v\f\r "
       "0x7F x123x a1b2c3 Les naïfs ægithales hâtifs pondant à Noël où il "
       "gèle sont sûrs d'être déçus en voyant leurs drôles d'œufs abîmés. "
       "Falsches Üben von Xylophonmusik quält jeden größeren Zwerg. ξεσκεπάζω "
       "την ψυχοφθόρα \\t\\n\\v\\f\\r βδελυγμία. いろはにほへど　ちりぬるを "
       "わがよたれぞ　つねならむ うゐのおくやま　けふこえて あさきゆめみじ　"
       "ゑひもせず",
       " The quick brown fox jumps over the lazy dog. "
       "$123,000.0 !\"#$%&'()*+,-./:<=>?@\\[]^_`{|}~ 0123456789 \t\n \f\r "
       "0x7F x123x a1b2c3 Les naïfs ægithales hâtifs pondant à Noël où il "
       "gèle sont sûrs d'être déçus en voyant leurs drôles d'œufs abîmés. "
       "Falsches Üben von Xylophonmusik quält jeden größeren Zwerg. ξεσκεπάζω "
       "την ψυχοφθόρα \\t\\n\\v\\f\\r βδελυγμία. いろはにほへど ちりぬるを "
       "わがよたれぞ つねならむ うゐのおくやま けふこえて あさきゆめみじ "
       "ゑひもせず"},  // Pangrams
      {"\u2002Test\u1680 \u2028String\u00A0\u3000",
       " Test   String  "},  // Non ASCII whitespaces
      {"A わ\x05\x04\x03\x02\x01Ü\x00 ABC123", "A わ     Ü"},
      {"\xEF\xBB\xBF-abc", " -abc"},  // U+FEFF used as UTF-8 BOM
      {"\xEF\xB7\x90 \xF4\x8F\xBF\xBE",
       "   "},                       // Non-characters U+FDD0 U+10FFFE
      {"\xF0\x8F\xBF\xBE", "    "},  // Invalid UTF8: invalid encoding of
                                     // U+1FFFE (0x8F instead of 0x9F)
      {"\xED\xA0\x80\xED\xBF\xBF",
       "      "},                    // Invalid UTF8: Surrogate code points
      {"\xE0\x80\x80", "   "},       // Invalid UTF8: Overlong sequences
      {"\xF4\x90\x80\x80", "    "},  // Invalid UTF8: Beyond U+10FFFF (the upper
                                     // limit of Unicode codespace)
      {"\xFE\xFF", "  "},            // Invalid UTF8: BOM in UTF-16(BE|LE)
      {"\xD9\xEE\xE4\xEE",
       "    "},  // Invalid UTF8: U+0639 U+064E U+0644 U+064E in ISO-8859-6
      {"\xef\xbb\xbf-abc", " -abc"},  // Invalid UTF8 mixed with valid UTF8
      {"", ""}};

  for (const auto& [text, expected_text] : samples) {
    EXPECT_EQ(expected_text, StripNonAlphanumericOrAsciiCharacters(text));
  }
}

TEST(StripNonAlphanumericAsciiCharactersTest, BinaryData) {
  const char bytes_data[] = {0x12, 0x11, 0x10, 0x09, 0x08, 0x07, 0x06,
                             0x05, 0x04, 0x03, 0x02, 0x01, 0x00, 0x21};
  const std::string binary_data(
      reinterpret_cast<const char*>(bytes_data),
      reinterpret_cast<const char*>(bytes_data) + sizeof(bytes_data));

  EXPECT_EQ("   \t         !",
            StripNonAlphanumericOrAsciiCharacters(binary_data));
}

}  // namespace text_sanitize
