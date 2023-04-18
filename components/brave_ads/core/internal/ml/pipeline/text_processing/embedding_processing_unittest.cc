/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/pipeline/text_processing/embedding_processing.h"

#include <tuple>
#include <vector>

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/resources/contextual/text_embedding/text_embedding_resource.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsEmbeddingProcessingTest : public UnitTestBase {};

TEST_F(BraveAdsEmbeddingProcessingTest, EmbedText) {
  // Arrange
  resource::TextEmbedding resource;
  resource.Load();
  task_environment_.RunUntilIdle();
  ASSERT_TRUE(resource.IsInitialized());

  const ml::pipeline::EmbeddingProcessing* const processing_pipeline =
      resource.Get();
  ASSERT_TRUE(processing_pipeline);

  const std::vector<std::tuple<std::string, std::vector<float>>> k_samples = {
      {"this simple unittest", {0.5F, 0.4F, 1.0F}},
      {"this is a simple unittest", {0.5F, 0.4F, 1.0F}},
      {"this is @ #1a simple unittest", {0.5F, 0.4F, 1.0F}},
      {"that is a test", {0.0F, 0.0F, 0.0F}},
      {"this 54 is simple", {0.85F, 0.2F, 1.0F}},
      {{}, {}}};

  for (const auto& [text, expected_embedding] : k_samples) {
    // Act
    const ml::pipeline::TextEmbeddingInfo text_embedding =
        processing_pipeline->EmbedText(text);

    // Assert
    EXPECT_EQ(expected_embedding, text_embedding.embedding);
  }
}

}  // namespace brave_ads
