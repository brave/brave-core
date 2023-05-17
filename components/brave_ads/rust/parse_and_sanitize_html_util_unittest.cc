/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/rust/parse_and_sanitize_html_util.h"

#include <tuple>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::rust {

TEST(BraveAdsParseAndSanitizeOgTagAttributeTest,
     ParseAndSanitizeOgTagAttribute) {
  // Arrange
  const std::vector<std::tuple<std::string, std::string>> samples = {
      {R"(<meta property="og:title" content="test">)", "test"},
      {R"(<meta property="og:title" content=" testing   ">)", "testing"},
      {R"(<meta property="og:title" content="test (string) - )"
       R"(for 78 unittest 246">)",
       "test string for unittest"},
      {R"(<meta property="og:title" content="Test this,string - )"
       R"(for UNiTTeST">)",
       "test this string for unittest"},
      {R"(<meta property="og:title" content="Test string, string,... )"
       R"(for unittest">)",
       "test string string for unittest"},
      {R"(<meta property="og:title" content="Test string1, string2,... )"
       R"(for unittest">)",
       "test for unittest"},
      {R"(<meta property="og:tt" content=" testing   ">)", {}},
      {R"(<meta property="og:title" cc=" testing   ">)", {}},
      {R"(<meta property="og:title" content="test")", {}},
      {R"(meta property="og:title" content="test">)", {}},
      {"<div>", {}},
      {"<>", {}},
      {" ", {}},
      {{}, {}}};

  for (const auto& [html, expected_text] : samples) {
    // Act

    // Assert
    EXPECT_EQ(expected_text, ParseAndSanitizeOgTagAttribute(html));
  }
}

}  // namespace brave_ads::rust
