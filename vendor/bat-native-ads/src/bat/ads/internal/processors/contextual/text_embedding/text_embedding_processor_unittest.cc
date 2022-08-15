/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_processor_util.h"

#include <map>

#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_file_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace processor {

class BatAdsTextEmbeddingProcessorTest : public UnitTestBase {
 protected:
  BatAdsTextEmbeddingProcessorTest() = default;

  ~BatAdsTextEmbeddingProcessorTest() override = default;
};

TEST_F(BatAdsTextEmbeddingProcessorTest, SanitizeTextHtml) {
  // Arrange
  const std::map<std::string, std::string> samples = {
      {"<meta property=\"og:title\" content=\"test\">", "test"},
      {"<meta property=\"og:title\" content=\" testing   \">", "testing"},
      {"<meta property=\"og:title\" content=\"test (string) - for 78 unittest "
       "246\">",
       "test string for unittest"},
      {"<meta property=\"og:title\" content=\"Test this,string - for "
       "UNiTTeST\">",
       "test this string for unittest"},
      {"<meta property=\"og:title\" content=\"Test string, string,... for "
       "unittest\">",
       "test string string for unittest"},
      {"<meta property=\"og:title\" content=\"Test string1, string2,... for "
       "unittest\">",
       "test for unittest"},
      {"<meta property=\"og:tt\" content=\" testing   \">", ""},
      {"<meta property=\"og:title\" cc=\" testing   \">", ""},
      {"<meta property=\"og:title\" content=\"test\"", ""},
      {"meta property=\"og:title\" content=\"test\">", ""}};

  for (auto const& sample : samples) {
    // Act
    std::string sanitized = SanitizeText(sample.first, true);
    // Assert
    ASSERT_EQ(sample.second, sanitized);
  }
}

}  // namespace processor
}  // namespace ads
