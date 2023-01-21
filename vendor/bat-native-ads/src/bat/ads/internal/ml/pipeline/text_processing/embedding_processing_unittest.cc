/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/pipeline/text_processing/embedding_processing.h"

#include <tuple>
#include <vector>

#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/ml/data/vector_data.h"
#include "bat/ads/internal/resources/contextual/text_embedding/text_embedding_resource.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kResourceFile[] = "wtpwsrqtjxmfdwaymauprezkunxprysm";
constexpr char kSimpleResourceFile[] =
    "resources/wtpwsrqtjxmfdwaymauprezkunxprysm_simple";

}  // namespace

class BatAdsEmbeddingProcessingTest : public UnitTestBase {};

TEST_F(BatAdsEmbeddingProcessingTest, EmbedText) {
  // Arrange
  CopyFileFromTestPathToTempPath(kSimpleResourceFile, kResourceFile);

  resource::TextEmbedding resource;
  resource.Load();

  task_environment_.RunUntilIdle();
  ASSERT_TRUE(resource.IsInitialized());

  const ml::pipeline::EmbeddingProcessing* const embedding_processing =
      resource.Get();
  ASSERT_TRUE(embedding_processing);

  const std::vector<std::tuple<std::string, ml::VectorData>> samples = {
      {"this simple unittest", ml::VectorData({0.5, 0.4, 1.0})},
      {"this is a simple unittest", ml::VectorData({0.5, 0.4, 1.0})},
      {"this is @ #1a simple unittest", ml::VectorData({0.5, 0.4, 1.0})},
      {"that is a test", ml::VectorData({0.0, 0.0, 0.0})},
      {"this 54 is simple", ml::VectorData({0.85, 0.2, 1.0})},
      {{}, {}}};

  for (const auto& [text, expected_embedding] : samples) {
    // Act
    const ml::pipeline::TextEmbeddingInfo text_embedding =
        embedding_processing->EmbedText(text);
    // Assert
    EXPECT_EQ(expected_embedding.GetValuesForTesting(),
              text_embedding.embedding.GetValuesForTesting());
  }
}

}  // namespace ads
