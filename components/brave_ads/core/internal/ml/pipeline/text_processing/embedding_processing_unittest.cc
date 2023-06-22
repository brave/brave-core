/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <tuple>
#include <vector>

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/resources/contextual/text_embedding/text_embedding_resource.h"
#include "brave/components/brave_ads/core/internal/resources/language_components_unittest_constants.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

using testing::_;

}  // namespace

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

  for (const auto& [text, expected_embedding] : k_samples) {
    base::MockCallback<EmbedTextCallback> embed_text_callback;
    EXPECT_CALL(embed_text_callback, Run(_))
        .WillOnce([&expected_embedding](
                      const ml::pipeline::TextEmbeddingInfo& text_embedding) {
          EXPECT_EQ(expected_embedding, text_embedding.embedding);
        });

    // Act
    resource_->EmbedText(text, embed_text_callback.Get());
    task_environment_.RunUntilIdle();
  }
}

}  // namespace brave_ads
