/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/pipeline/text_processing/embedding_processing.h"

#include <tuple>
#include <vector>

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/resources/contextual/text_embedding/text_embedding_resource.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

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

  const std::vector<std::tuple<std::string, std::vector<float>>> k_samples = {
      {"this simple unittest", {0.5, 0.4, 1.0}},
      {"this is a simple unittest", {0.5, 0.4, 1.0}},
      {"this is @ #1a simple unittest", {0.5, 0.4, 1.0}},
      {"that is a test", {0.0, 0.0, 0.0}},
      {"this 54 is simple", {0.85, 0.2, 1.0}},
      {{}, {}}};

  for (const auto& [text, expected_embedding] : k_samples) {
    // Act
    const ml::pipeline::TextEmbeddingInfo text_embedding =
        embedding_processing->EmbedText(text);
    // Assert
    EXPECT_EQ(expected_embedding, text_embedding.embedding);
  }
}

}  // namespace brave_ads
