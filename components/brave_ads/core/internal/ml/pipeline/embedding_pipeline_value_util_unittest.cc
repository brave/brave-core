/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/pipeline/embedding_pipeline_value_util.h"

#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/embedding_pipeline_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::ml::pipeline {

namespace {

constexpr char kJson[] =
    R"({"locale": "EN", "timestamp": "2022-06-09 08:00:00.704847", "version": 1, "embeddings": {"quick": [0.7481, 0.0493, -0.5572], "brown": [-0.0647, 0.4511, -0.7326], "fox": [-0.9328, -0.2578, 0.0032]}})";
constexpr char kJsonEmpty[] = "{}";
constexpr char kJsonMalformed[] =
    R"({"locale": "EN", "timestamp": "2022-06-09 08:00:00.704847", "version": 1, "embeddings": {"quick": "foobar"}})";

}  // namespace

class BatAdsEmbeddingPipelineValueUtilTest : public UnitTestBase {};

TEST_F(BatAdsEmbeddingPipelineValueUtilTest, FromValue) {
  // Arrange
  const base::Value value = base::test::ParseJson(kJson);
  const base::Value::Dict* const dict = value.GetIfDict();
  ASSERT_TRUE(dict);

  const std::vector<float> quick_data = {0.7481F, 0.0493F, -0.5572F};
  const std::vector<float> brown_data = {-0.0647F, 0.4511F, -0.7326F};
  const std::vector<float> fox_data = {-0.9328F, -0.2578F, 0.0032F};
  const std::vector<std::tuple<std::string, VectorData>> k_samples = {
      {"quick", VectorData(quick_data)},
      {"brown", VectorData(brown_data)},
      {"fox", VectorData(fox_data)},
  };

  // Act
  const absl::optional<EmbeddingPipelineInfo> pipeline =
      EmbeddingPipelineFromValue(*dict);
  ASSERT_TRUE(pipeline);
  EmbeddingPipelineInfo embedding_pipeline = *pipeline;

  for (const auto& [token, expected_embedding] : k_samples) {
    const auto iter = embedding_pipeline.embeddings.find(token);
    ASSERT_TRUE(iter != embedding_pipeline.embeddings.end());
    const VectorData& token_embedding_vector_data = iter->second;

    // Assert
    for (int i = 0; i < 3; i++) {
      EXPECT_NEAR(expected_embedding.GetData().at(i),
                  token_embedding_vector_data.GetData().at(i), 0.001F);
    }
  }
}

TEST_F(BatAdsEmbeddingPipelineValueUtilTest, FromValueEmpty) {
  // Arrange
  const base::Value value = base::test::ParseJson(kJsonEmpty);
  const base::Value::Dict* const dict = value.GetIfDict();
  ASSERT_TRUE(dict);

  // Act
  const absl::optional<EmbeddingPipelineInfo> pipeline =
      EmbeddingPipelineFromValue(*dict);

  // Assert
  EXPECT_TRUE(!pipeline);
}

TEST_F(BatAdsEmbeddingPipelineValueUtilTest, FromValueMalformed) {
  // Arrange
  const base::Value value = base::test::ParseJson(kJsonMalformed);
  const base::Value::Dict* const dict = value.GetIfDict();
  ASSERT_TRUE(dict);

  // Act
  const absl::optional<EmbeddingPipelineInfo> pipeline =
      EmbeddingPipelineFromValue(*dict);

  // Assert
  EXPECT_TRUE(!pipeline);
}

}  // namespace brave_ads::ml::pipeline
