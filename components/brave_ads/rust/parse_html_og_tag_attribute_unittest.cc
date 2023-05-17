/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <tuple>
#include <vector>

#include "brave/components/brave_ads/rust/lib.rs.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::rust {

namespace {

std::string ParseHtmlOgTagAttribute(const std::string& html) {
  return static_cast<std::string>(parse_html_og_tag_attribute(html));
}

}  // namespace

TEST(BraveAdsParseHtmlOgTagAttributeTest, ParseHtmlOgTagAttribute) {
  // Arrange
  const std::vector<std::tuple<
      /*html*/ std::string,
      /*expected_html_tag_attribute*/ std::string>>
      samples = {
          {R"(<meta property="og:title" content="this is info ">)",
           "this is info "},
          {R"(<meta  content="this is info " property="og:title">)",
           "this is info "},
          {R"(<meta property="og:title" content=' this is info '>)",
           " this is info "},
          {R"(<meta property="og:title" foo="bar" content="this is info ">)",
           "this is info "},
          {R"(<meta property="og:title" content=" this is info " foo="bar">)",
           " this is info "},
          {R"(<div href="brave.com" content="this is info ">)", {}},
          {R"(<meta notproperty="og:title" content="this is info">)", {}},
          {R"(<meta property="og:title" not_content="this is info">)", {}},
          {R"(<meta property="og:title" content=>)", {}},
          {R"(<meta property="og:title" content=">)", {}},
          {R"(<meta property="og:title" content="info'>)", {}},
          {R"(<meta property="og:title" content="info>)", {}},
          {R"(<meta property="og:title" content='info>)", {}},
          {R"(<meta property="og:title" content=info>)", {}},
          {R"(<div property="og:title" )"
           R"(content="The quick brown fox jumps over the lazy dog.">)",
           "The quick brown fox jumps over the lazy dog."},
          {R"(<div property="og:title")"
           R"(content="Les naïfs ægithales hâtifs pondant à Noël où il gèle )"
           R"(sont sûrs d'être déçus en voyant leurs drôles d'œufs abîmés.">)",
           "Les naïfs ægithales hâtifs pondant à Noël où il gèle sont sûrs "
           "d'être déçus en voyant leurs drôles d'œufs abîmés."},
          {R"(<div property="og:title" content="Falsches Üben von )"
           R"(Xylophonmusik quält jeden größeren Zwerg. ξεσκεπάζω την )"
           R"(ψυχοφθόρα βδελυγμία.">)",
           "Falsches Üben von Xylophonmusik quält jeden größeren Zwerg. "
           "ξεσκεπάζω την ψυχοφθόρα βδελυγμία."},
          {R"(<div property="og:title" content="いろはにほへど　ちりぬるを )"
           R"(わがよたれぞ　つねならむ うゐのおくやま　けふこえて あさきゆめみじ　)"
           R"(ゑひもせず">)",
           "いろはにほへど　ちりぬるを わがよたれぞ　つねならむ "
           "うゐのおくやま　けふこえて あさきゆめみじ　ゑひもせず"}};

  for (const auto& [html, expected_html_tag_attribute] : samples) {
    // Act
    const std::string html_tag_attribute = ParseHtmlOgTagAttribute(html);

    // Assert
    EXPECT_EQ(expected_html_tag_attribute, html_tag_attribute);
  }
}

}  // namespace brave_ads::rust
