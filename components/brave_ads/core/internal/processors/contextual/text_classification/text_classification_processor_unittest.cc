/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/processors/contextual/text_classification/text_classification_processor.h"

#include "brave/components/brave_ads/core/internal/ads/serving/targeting/contextual/text_classification/text_classification_alias.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/contextual/text_classification/text_classification_model.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/resources/contextual/text_classification/text_classification_resource.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsTextClassificationProcessorTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    resource_.Load();
    task_environment_.RunUntilIdle();
  }

  TextClassificationResource resource_;
};

TEST_F(BraveAdsTextClassificationProcessorTest,
       DoNotProcessIfResourceIsNotInitialized) {
  // Arrange
  TextClassificationResource resource;

  // Act
  TextClassificationProcessor processor(resource);
  processor.Process(/*text*/ "The quick brown fox jumps over the lazy dog");

  // Assert
  const TextClassificationProbabilityList& list =
      ClientStateManager::GetInstance()
          .GetTextClassificationProbabilitiesHistory();

  EXPECT_TRUE(list.empty());
}

TEST_F(BraveAdsTextClassificationProcessorTest, DoNotProcessForEmptyText) {
  // Act
  const std::string text;
  TextClassificationProcessor processor(resource_);
  processor.Process(text);

  // Assert
  const TextClassificationProbabilityList& list =
      ClientStateManager::GetInstance()
          .GetTextClassificationProbabilitiesHistory();

  EXPECT_TRUE(list.empty());
}

TEST_F(BraveAdsTextClassificationProcessorTest, NeverProcessed) {
  // Act
  const TextClassificationModel model;
  const SegmentList segments = model.GetSegments();

  // Assert
  const TextClassificationProbabilityList& list =
      ClientStateManager::GetInstance()
          .GetTextClassificationProbabilitiesHistory();

  EXPECT_TRUE(list.empty());
}

TEST_F(BraveAdsTextClassificationProcessorTest, ProcessText) {
  // Act
  TextClassificationProcessor processor(resource_);
  processor.Process(/*text*/ "Some content about technology & computing");

  // Assert
  const TextClassificationProbabilityList& list =
      ClientStateManager::GetInstance()
          .GetTextClassificationProbabilitiesHistory();

  EXPECT_EQ(1U, list.size());
}

TEST_F(BraveAdsTextClassificationProcessorTest, ProcessMultipleText) {
  // Act
  TextClassificationProcessor processor(resource_);
  processor.Process(/*text*/ "Some content about cooking food");
  processor.Process(/*text*/ "Some content about finance & banking");
  processor.Process(/*text*/ "Some content about technology & computing");

  // Assert
  const TextClassificationProbabilityList& list =
      ClientStateManager::GetInstance()
          .GetTextClassificationProbabilitiesHistory();

  EXPECT_EQ(3U, list.size());
}

}  // namespace brave_ads
