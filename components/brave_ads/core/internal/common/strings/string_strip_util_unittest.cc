/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/strings/string_strip_util.h"

#include "brave/components/brave_ads/core/internal/sanitize/sanitize_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsStringStripUtilTest : public ::testing::TestWithParam<bool> {
 protected:
  bool ShouldSanitizeHtmlContent() const { return GetParam(); }

  std::string MaybeSanitizeHtmlContent(const std::string& content) const {
    return ShouldSanitizeHtmlContent() ? SanitizeHtmlContent(content) : content;
  }
};

TEST_P(BraveAdsStringStripUtilTest, StripNonAlphaCharactersFromEmptyContent) {
  // Arrange
  const std::string text = "";
  const std::string sanitized_text = MaybeSanitizeHtmlContent(text);

  // Act
  const std::string stripped_text = StripNonAlphaCharacters(sanitized_text);

  // Assert
  EXPECT_TRUE(stripped_text.empty());
}

TEST_P(BraveAdsStringStripUtilTest, StripNonAlphaCharactersFromWhitespace) {
  // Arrange
  const std::string text = "   ";
  const std::string sanitized_text = MaybeSanitizeHtmlContent(text);

  // Act
  const std::string stripped_text = StripNonAlphaCharacters(sanitized_text);

  // Assert
  EXPECT_TRUE(stripped_text.empty());
}

TEST_P(BraveAdsStringStripUtilTest, StripNonAlphaCharacters) {
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
  const std::string sanitized_content = MaybeSanitizeHtmlContent(content);

  // Act
  const std::string stripped_content =
      StripNonAlphaCharacters(sanitized_content);

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

TEST_P(BraveAdsStringStripUtilTest,
       StripNonAlphaNumericCharactersFromEmptyContent) {
  // Arrange
  const std::string text = "";
  const std::string sanitized_text = MaybeSanitizeHtmlContent(text);

  // Act
  const std::string stripped_text =
      StripNonAlphaNumericCharacters(sanitized_text);

  // Assert
  EXPECT_TRUE(stripped_text.empty());
}

TEST_P(BraveAdsStringStripUtilTest,
       StripNonAlphaNumericCharactersFromWhitespace) {
  // Arrange
  const std::string text = "   ";
  const std::string sanitized_text = MaybeSanitizeHtmlContent(text);

  // Act
  const std::string stripped_text =
      StripNonAlphaNumericCharacters(sanitized_text);

  // Assert
  EXPECT_TRUE(stripped_text.empty());
}

TEST_P(BraveAdsStringStripUtilTest, StripNonAlphaNumericCharacters) {
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
  const std::string sanitized_content = MaybeSanitizeHtmlContent(content);

  // Act
  const std::string stripped_content =
      StripNonAlphaNumericCharacters(sanitized_content);

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

INSTANTIATE_TEST_SUITE_P(,
                         BraveAdsStringStripUtilTest,
                         ::testing::Bool(),
                         [](::testing::TestParamInfo<bool> test_param) {
                           return test_param.param ? "HtmlSanitized"
                                                   : "HtmlNotSanitized";
                         });

}  // namespace brave_ads
