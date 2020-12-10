/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/html_util.h"

#include <string>

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsHtmlUtilTest,
    StripHtmlTagsAndNonAlphaCharactersFromEmptyContent) {
  // Arrange
  const std::string content = "";

  // Act
  const std::string stripped_content =
      StripHtmlTagsAndNonAlphaCharacters(content);

  // Assert
  const std::string expected_stripped_content = "";

  EXPECT_EQ(expected_stripped_content, stripped_content);
}

TEST(BatAdsHtmlUtilTest,
    StripHtmlTagsAndNonAlphaCharactersFromWhitespace) {
  // Arrange
  const std::string content = "   ";

  // Act
  const std::string stripped_content =
      StripHtmlTagsAndNonAlphaCharacters(content);

  // Assert
  const std::string expected_stripped_content = "";

  EXPECT_EQ(expected_stripped_content, stripped_content);
}

TEST(BatAdsHtmlUtilTest,
    StripHtmlTagsAndNonAlphaCharacters) {
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

  // Act
  const std::string stripped_content =
      StripHtmlTagsAndNonAlphaCharacters(content);

  // Assert
  const std::string expected_stripped_content =
      "The quick brown fox jumps over the lazy dog Les naïfs ægithales hâtifs "
      "pondant à Noël où il gèle sont sûrs d être déçus en voyant leurs drôles "
      "d œufs abîmés Falsches Üben von Xylophonmusik quält jeden größeren "
      "Zwerg ξεσκεπάζω την ψυχοφθόρα βδελυγμία いろはにほへど ちりぬるを "
      "わがよたれぞ つねならむ うゐのおくやま けふこえて あさきゆめみじ "
      "ゑひもせず";

  EXPECT_EQ(expected_stripped_content, stripped_content);
}

TEST(BatAdsHtmlUtilTest,
    StripHtmlTagsAndNonAlphaNumericCharactersFromEmptyContent) {
  // Arrange
  const std::string content = "";

  // Act
  const std::string stripped_content =
      StripHtmlTagsAndNonAlphaNumericCharacters(content);

  // Assert
  const std::string expected_stripped_content = "";

  EXPECT_EQ(expected_stripped_content, stripped_content);
}

TEST(BatAdsHtmlUtilTest,
    StripHtmlTagsAndNonAlphaNumericCharactersFromWhitespace) {
  // Arrange
  const std::string content = "   ";

  // Act
  const std::string stripped_content =
      StripHtmlTagsAndNonAlphaNumericCharacters(content);

  // Assert
  const std::string expected_stripped_content = "";

  EXPECT_EQ(expected_stripped_content, stripped_content);
}

TEST(BatAdsHtmlUtilTest,
    StripHtmlTagsAndNonAlphaNumericCharacters) {
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

  // Act
  const std::string stripped_content =
      StripHtmlTagsAndNonAlphaNumericCharacters(content);

  // Assert
  const std::string expected_stripped_content =
      "The quick brown fox jumps over the lazy dog 123 000 0 0123456789 0x7F "
      "x123x a1b2c3 Les naïfs ægithales hâtifs pondant à Noël où il gèle sont "
      "sûrs d être déçus en voyant leurs drôles d œufs abîmés Falsches Üben "
      "von Xylophonmusik quält jeden größeren Zwerg ξεσκεπάζω την ψυχοφθόρα "
      "βδελυγμία いろはにほへど ちりぬるを わがよたれぞ つねならむ "
      "うゐのおくやま けふこえて あさきゆめみじ ゑひもせず";

  EXPECT_EQ(expected_stripped_content, stripped_content);
}

}  // namespace ads
