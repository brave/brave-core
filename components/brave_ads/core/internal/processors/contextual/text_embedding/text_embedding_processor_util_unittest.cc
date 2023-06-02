/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/processors/contextual/text_embedding/text_embedding_processor_util.h"

#include <tuple>
#include <vector>

#include "brave/components/brave_ads/core/internal/sanitize/sanitize_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsTextEmbeddingProcessorUtilTest
    : public ::testing::TestWithParam<bool> {
 protected:
  bool ShouldSanitizeHtmlContent() const { return GetParam(); }

  std::string MaybeSanitizeHtmlContent(const std::string& content) const {
    return ShouldSanitizeHtmlContent() ? SanitizeHtmlContent(content) : content;
  }
};

TEST_P(BraveAdsTextEmbeddingProcessorUtilTest,
       ParseAndSanitizeHtmlTagAttribute) {
  // Arrange
  const std::vector<std::tuple<std::string, std::string>> samples = {
      {R"(<meta property="og:title" content="test">)", "test"},
      {R"(<meta property="og:title" content=" testing   ">)", "testing"},
      {R"(<meta property="og:title" content="test (string) - for 78 unittest 246">)",
       "test string for unittest"},
      {R"(<meta property="og:title" content="Test this,string - for UNiTTeST">)",
       "test this string for unittest"},
      {R"(<meta property="og:title" content="Test string, string,... for unittest">)",
       "test string string for unittest"},
      {R"(<meta property="og:title" content="Test string1, string2,... for unittest">)",
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
    const std::string sanitized_html = MaybeSanitizeHtmlContent(html);
    EXPECT_EQ(expected_text, ParseAndSanitizeHtmlTagAttribute(sanitized_html));
  }
}

TEST_P(BraveAdsTextEmbeddingProcessorUtilTest, SanitizeText) {
  // Arrange
  const std::vector<std::tuple<std::string, std::string>> samples = {
      {"test", "test"},
      {" testing   ", "testing"},
      {"test (string) - for 78 unittest 246", "test string for unittest"},
      {"Test this,string - for UNiTTeST", "test this string for unittest"},
      {"Test string, string,... for unittest",
       "test string string for unittest"},
      {"Test string1, string2,... for unittest", "test for unittest"},
      {"321", {}},
      {"<>", {}},
      {" ", {}},
      {{}, {}}};

  for (const auto& [text, expected_sanitized_text] : samples) {
    // Act

    // Assert
    const std::string sanitized_text = MaybeSanitizeHtmlContent(text);
    EXPECT_EQ(expected_sanitized_text, SanitizeText(sanitized_text));
  }
}

INSTANTIATE_TEST_SUITE_P(,
                         BraveAdsTextEmbeddingProcessorUtilTest,
                         ::testing::Bool(),
                         [](::testing::TestParamInfo<bool> test_param) {
                           return test_param.param ? "HtmlSanitized"
                                                   : "HtmlNotSanitized";
                         });

}  // namespace brave_ads
