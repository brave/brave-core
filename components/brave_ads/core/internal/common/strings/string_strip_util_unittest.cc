/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/strings/string_strip_util.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsStringStripUtilTest, StripNonAlphaCharactersFromEmptyContent) {
  // Act & Assert
  const std::string stripped_text = StripNonAlphaCharacters("");
  EXPECT_TRUE(stripped_text.empty());
}

TEST(BraveAdsStringStripUtilTest, StripNonAlphaCharactersFromWhitespace) {
  // Act & Assert
  const std::string stripped_text = StripNonAlphaCharacters("   ");
  EXPECT_TRUE(stripped_text.empty());
}

TEST(BraveAdsStringStripUtilTest, StripNonAlphaCharacters) {
  // Arrange
  const std::string content =
      "  The quick brown fox jumps over the lazy dog. "
      "$123,000.0 !\"#$%&'()*+,-./:<=>?@\\[]^_`{|}~ 0123456789 \t\n\v\f\r "
      "0x7F x123x a1b2c3 Les naïfs ægithales hâtifs pondant à Noël où il "
      "gèle sont sûrs d'être déçus en voyant leurs drôles d'œufs abîmés. "
      "Falsches Üben von Xylophonmusik quält jeden größeren Zwerg. ξεσκεπάζω "
      "την ψυχοφθόρα \\t\\n\\v\\f\\r βδελυγμία. いろはにほへど　ちりぬるを "
      "わがよたれぞ　つねならむ うゐのおくやま　けふこえて あさきゆめみじ　"
      "ゑひもせず  ";  // The Quick Brown Fox... Pangrams

  // Act & Assert
  const std::string expected_stripped_content =
      "The quick brown fox jumps over the lazy dog Les naïfs ægithales hâtifs "
      "pondant à Noël où il gèle sont sûrs d être déçus en voyant leurs drôles "
      "d œufs abîmés Falsches Üben von Xylophonmusik quält jeden größeren "
      "Zwerg ξεσκεπάζω την ψυχοφθόρα βδελυγμία いろはにほへど ちりぬるを "
      "わがよたれぞ つねならむ うゐのおくやま けふこえて あさきゆめみじ "
      "ゑひもせず";
  EXPECT_EQ(expected_stripped_content, StripNonAlphaCharacters(content));
}

TEST(BraveAdsStringStripUtilTest,
     StripNonAlphaNumericCharactersFromEmptyContent) {
  // Act & Assert
  const std::string stripped_text = StripNonAlphaNumericCharacters("");
  EXPECT_TRUE(stripped_text.empty());
}

TEST(BraveAdsStringStripUtilTest,
     StripNonAlphaNumericCharactersFromWhitespace) {
  // Act & Assert
  const std::string stripped_text = StripNonAlphaNumericCharacters("   ");
  EXPECT_TRUE(stripped_text.empty());
}

TEST(BraveAdsStringStripUtilTest, StripNonAlphaNumericCharacters) {
  // Arrange
  const std::string content =
      "  The quick brown fox jumps over the lazy dog. "
      "$123,000.0 !\"#$%&'()*+,-./:<=>?@\\[]^_`{|}~ 0123456789 \t\n\v\f\r "
      "0x7F x123x a1b2c3 Les naïfs ægithales hâtifs pondant à Noël où il "
      "gèle sont sûrs d'être déçus en voyant leurs drôles d'œufs abîmés. "
      "Falsches Üben von Xylophonmusik quält jeden größeren Zwerg. ξεσκεπάζω "
      "την ψυχοφθόρα \\t\\n\\v\\f\\r βδελυγμία. いろはにほへど　ちりぬるを "
      "わがよたれぞ　つねならむ うゐのおくやま　けふこえて あさきゆめみじ　"
      "ゑひもせず  ";  // The Quick Brown Fox... Pangrams

  // Act & Assert
  const std::string expected_stripped_content =
      "The quick brown fox jumps over the lazy dog 123 000 0 0123456789 0x7F "
      "x123x a1b2c3 Les naïfs ægithales hâtifs pondant à Noël où il gèle sont "
      "sûrs d être déçus en voyant leurs drôles d œufs abîmés Falsches Üben "
      "von Xylophonmusik quält jeden größeren Zwerg ξεσκεπάζω την ψυχοφθόρα "
      "βδελυγμία いろはにほへど ちりぬるを わがよたれぞ つねならむ "
      "うゐのおくやま けふこえて あさきゆめみじ ゑひもせず";
  EXPECT_EQ(expected_stripped_content, StripNonAlphaNumericCharacters(content));
}

}  // namespace brave_ads
