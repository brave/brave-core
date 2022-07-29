/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/pipeline/text_processing/embedding_processing.h"

#include <algorithm>
#include <map>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/check.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "base/values.h"
#include "bat/ads/internal/base/crypto/crypto_util.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/base/strings/string_html_parse_util.h"
#include "bat/ads/internal/base/strings/string_strip_util.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_file_util.h"
#include "bat/ads/internal/ml/data/vector_data.h"
#include "bat/ads/internal/ml/pipeline/pipeline_embedding_info.h"
#include "bat/ads/internal/ml/pipeline/pipeline_util.h"
#include "bat/ads/internal/ml/pipeline/text_processing/embedding_data.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace ml {

class BatAdsEmbeddingProcessingPipelineTest : public UnitTestBase {
 protected:
  BatAdsEmbeddingProcessingPipelineTest() = default;

  ~BatAdsEmbeddingProcessingPipelineTest() override = default;
};

TEST_F(BatAdsEmbeddingProcessingPipelineTest, SanitizeTextHtml) {
  // Arrange
  pipeline::EmbeddingProcessing embedding_processing;
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
    std::string sanitized =
        embedding_processing.SanitizeText(sample.first, true);  // Act
    ASSERT_EQ(sample.second, sanitized);                        // Assert
  }
}

TEST_F(BatAdsEmbeddingProcessingPipelineTest, EmbedTextSimple) {
  // Arrange
  pipeline::EmbeddingProcessing embedding_processing;
  pipeline::PipelineEmbeddingInfo pipeline_embedding;

  const std::map<std::string, VectorData> embeddings = {
      {"this", VectorData({1.0, 0.5, 0.7})},
      {"unittest", VectorData({-0.2, 0.8, 1.0})},
      {"simple", VectorData({0.7, -0.1, 1.3})}};

  pipeline_embedding.version = 1;
  pipeline_embedding.timestamp = base::Time::Now();
  pipeline_embedding.locale = "en";
  pipeline_embedding.dim = 3;
  pipeline_embedding.embeddings = embeddings;

  embedding_processing.SetEmbeddingPipeline(pipeline_embedding);
  embedding_processing.SetIsInitialized(true);

  const std::map<std::string, VectorData> samples = {
      {"this simple unittest", VectorData({0.5, 0.4, 1.0})},
      {"this is a simple unittest", VectorData({0.5, 0.4, 1.0})},
      {"that is a test", VectorData({0.0, 0.0, 0.0})},
      {"this 54 is simple", VectorData({0.85, 0.2, 1.0})},
      {"", VectorData({0.0, 0.0, 0.0})}};

  for (auto const& sample : samples) {
    pipeline::TextEmbeddingData embedding_data =
        embedding_processing.EmbedText(sample.first);  // Act
    EXPECT_EQ(sample.second.GetValuesForTesting(),
              embedding_data.embedding.GetValuesForTesting());  // Assert
  }
}

}  // namespace ml
}  // namespace ads
