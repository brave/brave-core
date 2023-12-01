/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/pipeline/text_processing/embedding_processing.h"

#include <memory>
#include <optional>
#include <tuple>
#include <vector>

#include "brave/components/brave_ads/core/internal/common/resources/language_components_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/resource/text_embedding_resource.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsEmbeddingProcessingTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    resource_ = std::make_unique<TextEmbeddingResource>();
  }

  bool LoadResource() {
    NotifyDidUpdateResourceComponent(kLanguageComponentManifestVersion,
                                     kLanguageComponentId);
    task_environment_.RunUntilIdle();
    return resource_->IsInitialized();
  }

  std::unique_ptr<TextEmbeddingResource> resource_;
};

TEST_F(BraveAdsEmbeddingProcessingTest, EmbedText) {
  // Arrange
  ASSERT_TRUE(LoadResource());

  const std::vector<std::tuple<std::string, std::vector<float>>> k_samples = {
      {"this simple unittest", {0.5F, 0.4F, 1.0F}},
      {"this is a simple unittest", {0.5F, 0.4F, 1.0F}},
      {"this is @ #1a simple unittest", {0.5F, 0.4F, 1.0F}},
      {"that is a test", {0.0F, 0.0F, 0.0F}},
      {"this 54 is simple", {0.85F, 0.2F, 1.0F}},
      {{}, {}}};

  const std::optional<ml::pipeline::EmbeddingProcessing>& embedding_processing =
      resource_->get();
  ASSERT_TRUE(embedding_processing);

  for (const auto& [text, expected_embedding] : k_samples) {
    // Act
    const ml::pipeline::TextEmbeddingInfo text_embedding =
        embedding_processing->EmbedText(text);

    // Assert
    EXPECT_EQ(expected_embedding, text_embedding.embedding);
  }
}

}  // namespace brave_ads
