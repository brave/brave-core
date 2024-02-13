/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/strings/string_html_parser_util.h"

#include <tuple>
#include <vector>

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsStringHtmlParserUtilTest : public UnitTestBase {};

TEST_F(BraveAdsStringHtmlParserUtilTest, ParseHtmlTagNameAttributeSimple) {
  // Arrange
  const std::string html_meta_tag =
      R"(<meta property="og:title" content="this is info ">)";
  const std::string html_meta_with_foobar_tag =
      R"(<meta property="og:title" foo="bar" content="this is info ">)";
  const std::string non_html_meta_tag =
      R"(<div href="brave.com" content="this is info ">)";
  const std::vector<std::tuple<
      /*html=*/std::string, /*tag_substr=*/std::string,
      /*tag_attribute=*/std::string,
      /*expected_html_tag_attribute=*/std::string>>
      samples = {
          {html_meta_tag, "og:title", "content", "this is info "},
          {html_meta_tag, "title", "content", "this is info "},
          {html_meta_tag, "title", "foo", {}},
          {html_meta_with_foobar_tag, "og:title", "content", "this is info "},
          {html_meta_with_foobar_tag, "og:title", "foo", "bar"},
          {non_html_meta_tag, "og:title", "content", {}},
          {non_html_meta_tag, "href", "content", "this is info "},
          {non_html_meta_tag, "href", "foo", {}},
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
    // Act & Assert
    EXPECT_EQ(expected_html_tag_attribute,
              ParseHtmlTagNameAttribute(html, tag_substr, tag_attribute));
  }
}

}  // namespace brave_ads
