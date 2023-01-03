/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/processors/contextual/text_classification/text_classification_processor.h"

#include "bat/ads/internal/ads/serving/targeting/models/contextual/text_classification/text_classification_alias.h"
#include "bat/ads/internal/ads/serving/targeting/models/contextual/text_classification/text_classification_model.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/deprecated/client/client_state_manager.h"
#include "bat/ads/internal/resources/contextual/text_classification/text_classification_resource.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsTextClassificationProcessorTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    resource_.Load();
    task_environment_.RunUntilIdle();
  }

  resource::TextClassification resource_;
};

TEST_F(BatAdsTextClassificationProcessorTest,
       DoNotProcessIfResourceIsNotInitialized) {
  // Arrange
  resource::TextClassification uninitialized_resource;

  // Act
  const std::string text = "The quick brown fox jumps over the lazy dog";
  processor::TextClassification processor(&uninitialized_resource);
  processor.Process(text);

  // Assert
  const targeting::TextClassificationProbabilityList& list =
      ClientStateManager::GetInstance()
          ->GetTextClassificationProbabilitiesHistory();

  EXPECT_TRUE(list.empty());
}

TEST_F(BatAdsTextClassificationProcessorTest, DoNotProcessForEmptyText) {
  // Act
  const std::string text;
  processor::TextClassification processor(&resource_);
  processor.Process(text);

  // Assert
  const targeting::TextClassificationProbabilityList& list =
      ClientStateManager::GetInstance()
          ->GetTextClassificationProbabilitiesHistory();

  EXPECT_TRUE(list.empty());
}

TEST_F(BatAdsTextClassificationProcessorTest, NeverProcessed) {
  // Act
  const targeting::model::TextClassification model;
  const SegmentList segments = model.GetSegments();

  // Assert
  const targeting::TextClassificationProbabilityList& list =
      ClientStateManager::GetInstance()
          ->GetTextClassificationProbabilitiesHistory();

  EXPECT_TRUE(list.empty());
}

TEST_F(BatAdsTextClassificationProcessorTest, ProcessText) {
  // Act
  const std::string text = "Some content about technology & computing";
  processor::TextClassification processor(&resource_);
  processor.Process(text);

  // Assert
  const targeting::TextClassificationProbabilityList& list =
      ClientStateManager::GetInstance()
          ->GetTextClassificationProbabilitiesHistory();

  EXPECT_EQ(1UL, list.size());
}

TEST_F(BatAdsTextClassificationProcessorTest, ProcessMultipleText) {
  // Act
  processor::TextClassification processor(&resource_);

  const std::string text_1 = "Some content about cooking food";
  processor.Process(text_1);

  const std::string text_2 = "Some content about finance & banking";
  processor.Process(text_2);

  const std::string text_3 = "Some content about technology & computing";
  processor.Process(text_3);

  // Assert
  const targeting::TextClassificationProbabilityList& list =
      ClientStateManager::GetInstance()
          ->GetTextClassificationProbabilitiesHistory();

  EXPECT_EQ(3UL, list.size());
}

}  // namespace ads
