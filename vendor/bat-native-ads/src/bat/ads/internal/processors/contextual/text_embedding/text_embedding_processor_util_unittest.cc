/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_processor_util.h"

#include <map>

#include "bat/ads/internal/base/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::processor {

class BatAdsTextEmbeddingProcessorUtilTest : public UnitTestBase {
 protected:
  BatAdsTextEmbeddingProcessorUtilTest() = default;

  ~BatAdsTextEmbeddingProcessorUtilTest() override = default;
};

TEST_F(BatAdsTextEmbeddingProcessorUtilTest, SanitizeHtml) {
  // Arrange
  const std::map<std::string, std::string> samples = {
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
      {R"(<div>)", {}},
      {R"(<>)", {}},
      {R"( )", {}},
      {R"()", {}}};

  for (const auto& [key, value] : samples) {
    // Act
    const std::string sanitized_html = SanitizeHtml(key);
    // Assert
    EXPECT_EQ(value, sanitized_html);
  }
}

TEST_F(BatAdsTextEmbeddingProcessorUtilTest, SanitizeText) {
  // Arrange
  const std::map<std::string, std::string> samples = {
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
      {"", {}}};

  for (const auto& [key, value] : samples) {
    // Act
    const std::string sanitized_text = SanitizeText(key);
    // Assert
    EXPECT_EQ(value, sanitized_text);
  }
}

}  // namespace ads::processor
