/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/text_classification_processor.h"

#include <memory>

#include "brave/components/brave_ads/core/internal/common/resources/language_components_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/model/text_classification_alias.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/resource/text_classification_resource.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsTextClassificationProcessorTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    resource_ = std::make_unique<TextClassificationResource>();
  }

  bool LoadResource() {
    NotifyDidUpdateResourceComponent(kLanguageComponentManifestVersion,
                                     kLanguageComponentId);
    task_environment_.RunUntilIdle();
    return resource_->IsInitialized();
  }

  std::unique_ptr<TextClassificationResource> resource_;
};

TEST_F(BraveAdsTextClassificationProcessorTest,
       DoNotProcessIfResourceIsNotInitialized) {
  // Arrange
  TextClassificationProcessor processor(*resource_);

  // Act
  processor.Process(/*text=*/"The quick brown fox jumps over the lazy dog");

  // Assert
  const TextClassificationProbabilityList& text_classification_probabilities =
      ClientStateManager::GetInstance()
          .GetTextClassificationProbabilitiesHistory();
  EXPECT_TRUE(text_classification_probabilities.empty());
}

TEST_F(BraveAdsTextClassificationProcessorTest, DoNotProcessForEmptyText) {
  // Arrange
  ASSERT_TRUE(LoadResource());

  TextClassificationProcessor processor(*resource_);

  // Act
  processor.Process(/*text=*/{});

  // Assert
  const TextClassificationProbabilityList& text_classification_probabilities =
      ClientStateManager::GetInstance()
          .GetTextClassificationProbabilitiesHistory();
  EXPECT_TRUE(text_classification_probabilities.empty());
}

TEST_F(BraveAdsTextClassificationProcessorTest, NeverProcessed) {
  // Arrange
  ASSERT_TRUE(LoadResource());

  // Act & Assert
  const TextClassificationProbabilityList& text_classification_probabilities =
      ClientStateManager::GetInstance()
          .GetTextClassificationProbabilitiesHistory();
  EXPECT_TRUE(text_classification_probabilities.empty());
}

TEST_F(BraveAdsTextClassificationProcessorTest, ProcessText) {
  // Arrange
  ASSERT_TRUE(LoadResource());

  TextClassificationProcessor processor(*resource_);

  // Act
  processor.Process(/*text=*/"Some content about technology & computing");

  // Assert
  const TextClassificationProbabilityList& text_classification_probabilities =
      ClientStateManager::GetInstance()
          .GetTextClassificationProbabilitiesHistory();
  EXPECT_EQ(1U, text_classification_probabilities.size());
}

TEST_F(BraveAdsTextClassificationProcessorTest, ProcessMultipleText) {
  // Arrange
  ASSERT_TRUE(LoadResource());

  TextClassificationProcessor processor(*resource_);

  // Act
  processor.Process(/*text=*/"Some content about cooking food");
  processor.Process(/*text=*/"Some content about finance & banking");
  processor.Process(/*text=*/"Some content about technology & computing");

  // Assert
  const TextClassificationProbabilityList& text_classification_probabilities =
      ClientStateManager::GetInstance()
          .GetTextClassificationProbabilitiesHistory();
  EXPECT_EQ(3U, text_classification_probabilities.size());
}

}  // namespace brave_ads
