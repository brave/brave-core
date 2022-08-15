/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/pipeline/text_processing/embedding_processing.h"

#include <map>
#include <vector>

#include "base/time/time.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_file_util.h"
#include "bat/ads/internal/ml/data/vector_data.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace ml {

class BatAdsEmbeddingProcessingPipelineTest : public UnitTestBase {
 protected:
  BatAdsEmbeddingProcessingPipelineTest() = default;

  ~BatAdsEmbeddingProcessingPipelineTest() override = default;
};

TEST_F(BatAdsEmbeddingProcessingPipelineTest, EmbedTextSimple) {
  // Arrange
  pipeline::EmbeddingProcessing embedding_processing;

  const int version = 1;
  const base::Time timestamp = base::Time::Now();
  const std::string& locale = "en";
  const int dim = 3;
  const std::map<std::string, VectorData> embeddings = {
      {"this", VectorData({1.0, 0.5, 0.7})},
      {"unittest", VectorData({-0.2, 0.8, 1.0})},
      {"simple", VectorData({0.7, -0.1, 1.3})}};

  embedding_processing.SetEmbeddingPipelineForTesting(version,
                                timestamp,
                                locale,
                                dim,
                                embeddings);

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
