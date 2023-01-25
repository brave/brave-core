/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/common/strings/string_html_parser_util.h"

#include <string>
#include <tuple>
#include <vector>

#include "bat/ads/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsStringHtmlParserUtilTest : public UnitTestBase {};

TEST_F(BatAdsStringHtmlParserUtilTest, ParseHtmlTagAttributeSimple) {
  // Arrange
  const std::string meta_html_tag =
      R"(<meta property="og:title" content="this is info ">)";
  const std::string meta_html_with_foobar_tag =
      R"(<meta property="og:title" foo="bar" content="this is info ">)";
  const std::string non_meta_html_tag =
      R"(<div href="brave.com" content="this is info ">)";
  const std::vector<std::tuple<
      /*html*/ std::string, /*tag_substr*/ std::string,
      /*tag_attribute*/ std::string,
      /*expected_html_tag_attribute*/ std::string>>
      samples = {
          {meta_html_tag, "og:title", "content", "this is info "},
          {meta_html_tag, "title", "content", "this is info "},
          {meta_html_tag, "title", "foo", {}},
          {meta_html_with_foobar_tag, "og:title", "content", "this is info "},
          {meta_html_with_foobar_tag, "og:title", "foo", "bar"},
          {non_meta_html_tag, "og:title", "content", {}},
          {non_meta_html_tag, "href", "content", "this is info "},
          {non_meta_html_tag, "href", "foo", {}},
          {R"(<div property="og:title" )"
           R"(content="The quick brown fox jumps over the lazy dog.">)",
           "og:title", "content",
           "The quick brown fox jumps over the lazy dog."},
          {R"(<div property="og:title")"
           R"(content="Les naïfs ægithales hâtifs pondant à Noël où il gèle )"
           R"(sont sûrs d'être déçus en voyant leurs drôles d'œufs abîmés.">)",
           "og:title", "content",
           "Les naïfs ægithales hâtifs pondant à Noël où il gèle sont sûrs "
           "d'être déçus en voyant leurs drôles d'œufs abîmés."},
          {R"(<div property="og:title" content="Falsches Üben von )"
           R"(Xylophonmusik quält jeden größeren Zwerg. ξεσκεπάζω την )"
           R"(ψυχοφθόρα βδελυγμία.">)",
           "og:title", "content",
           "Falsches Üben von Xylophonmusik quält jeden größeren Zwerg. "
           "ξεσκεπάζω την ψυχοφθόρα βδελυγμία."},
          {R"(<div property="og:title" content="いろはにほへど　ちりぬるを )"
           R"(わがよたれぞ　つねならむ うゐのおくやま　けふこえて あさきゆめみじ　)"
           R"(ゑひもせず">)",
           "og:title", "content",
           "いろはにほへど　ちりぬるを わがよたれぞ　つねならむ "
           "うゐのおくやま　けふこえて あさきゆめみじ　ゑひもせず"}};

  for (const auto& [html, tag_substr, tag_attribute,
                    expected_html_tag_attribute] : samples) {
    // Act
    const std::string html_tag_attribute =
        ParseHtmlTagAttribute(html, tag_substr, tag_attribute);
    // Assert
    EXPECT_EQ(expected_html_tag_attribute, html_tag_attribute);
  }
}

}  // namespace ads
