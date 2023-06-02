/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/strings/string_util.h"
#include "brave/components/text_sanitize/rs/src/lib.rs.h"
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
       "ゑひもせず"},
      {"\u2002Test\u1680 \u2028String\u00A0\u3000", " Test   String  "},
      {"A わ\x05\x04\x03\x02\x01Ü\x00 ABC123", "A わ     Ü"},
      {"\xEF\xBB\xBF-abc", " -abc"},  // U+FEFF used as UTF-8 BOM
      {"\xEF\xB7\x90 \xF4\x8F\xBF\xBE",
       "   "},  // Non-characters U+FDD0 U+10FFFE
      {"", ""}};

  for (const auto& [text, expected_text] : samples) {
    EXPECT_TRUE(base::IsStringUTF8AllowingNoncharacters(text));
    EXPECT_EQ(expected_text,
              static_cast<std::string>(
                  strip_non_alphanumeric_or_ascii_characters(text)));
  }
}

TEST(StripNonAlphanumericAsciiCharactersTest, BinaryData) {
  const char bytes_data[] = {0x12, 0x11, 0x10, 0x09, 0x08, 0x07, 0x06,
                             0x05, 0x04, 0x03, 0x02, 0x01, 0x00, 0x21};
  const std::string binary_data(
      reinterpret_cast<const char*>(bytes_data),
      reinterpret_cast<const char*>(bytes_data) + sizeof(bytes_data));

  EXPECT_EQ("   \t         !",
            static_cast<std::string>(
                strip_non_alphanumeric_or_ascii_characters(binary_data)));
}

}  // namespace text_sanitize
