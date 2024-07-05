/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/text_classification_processor.h"

#include <memory>

#include "brave/components/brave_ads/core/internal/common/resources/language_components_test_constants.h"
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

  std::unique_ptr<TextClassificationResource> resource_;
};

TEST_F(BraveAdsTextClassificationProcessorTest,
       DoNotProcessIfResourceIsNotInitialized) {
  // Arrange
  TextClassificationProcessor processor(*resource_);

  // Act
  processor.Process(/*text=*/"The quick brown fox jumps over the lazy dog");
  task_environment_.RunUntilIdle();

  // Assert
  const TextClassificationProbabilityList& text_classification_probabilities =
      ClientStateManager::GetInstance()
          .GetTextClassificationProbabilitiesHistory();
  EXPECT_THAT(text_classification_probabilities, ::testing::IsEmpty());
}

TEST_F(BraveAdsTextClassificationProcessorTest, DoNotProcessForEmptyText) {
  // Arrange
  NotifyDidUpdateResourceComponent(kLanguageComponentManifestVersion,
                                   kLanguageComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  TextClassificationProcessor processor(*resource_);

  // Act
  processor.Process(/*text=*/"");
  task_environment_.RunUntilIdle();

  // Assert
  const TextClassificationProbabilityList& text_classification_probabilities =
      ClientStateManager::GetInstance()
          .GetTextClassificationProbabilitiesHistory();
  EXPECT_THAT(text_classification_probabilities, ::testing::IsEmpty());
}

TEST_F(BraveAdsTextClassificationProcessorTest, NeverProcessed) {
  // Arrange
  NotifyDidUpdateResourceComponent(kLanguageComponentManifestVersion,
                                   kLanguageComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  // Act & Assert
  const TextClassificationProbabilityList& text_classification_probabilities =
      ClientStateManager::GetInstance()
          .GetTextClassificationProbabilitiesHistory();
  EXPECT_THAT(text_classification_probabilities, ::testing::IsEmpty());
}

TEST_F(BraveAdsTextClassificationProcessorTest, ProcessText) {
  // Arrange
  NotifyDidUpdateResourceComponent(kLanguageComponentManifestVersion,
                                   kLanguageComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  TextClassificationProcessor processor(*resource_);

  // Act
  processor.Process(/*text=*/"Some content about technology & computing");
  task_environment_.RunUntilIdle();

  // Assert
  const TextClassificationProbabilityList& text_classification_probabilities =
      ClientStateManager::GetInstance()
          .GetTextClassificationProbabilitiesHistory();
  EXPECT_THAT(text_classification_probabilities, ::testing::SizeIs(1));
}

TEST_F(BraveAdsTextClassificationProcessorTest, ProcessMultipleText) {
  // Arrange
  NotifyDidUpdateResourceComponent(kLanguageComponentManifestVersion,
                                   kLanguageComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  TextClassificationProcessor processor(*resource_);

  // Act
  processor.Process(/*text=*/"Some content about cooking food");
  processor.Process(/*text=*/"Some content about finance & banking");
  processor.Process(/*text=*/"Some content about technology & computing");
  task_environment_.RunUntilIdle();

  // Assert
  const TextClassificationProbabilityList& text_classification_probabilities =
      ClientStateManager::GetInstance()
          .GetTextClassificationProbabilitiesHistory();
  EXPECT_THAT(text_classification_probabilities, ::testing::SizeIs(3));
}

}  // namespace brave_ads
