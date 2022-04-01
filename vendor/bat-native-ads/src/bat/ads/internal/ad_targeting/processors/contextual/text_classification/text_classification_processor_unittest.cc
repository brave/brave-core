/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/processors/contextual/text_classification/text_classification_processor.h"

#include "bat/ads/internal/ad_serving/ad_targeting/models/contextual/text_classification/text_classification_model.h"
#include "bat/ads/internal/ad_targeting/data_types/contextual/text_classification/text_classification_aliases.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/resources/contextual/text_classification/text_classification_resource.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace ad_targeting {

class BatAdsTextClassificationProcessorTest : public UnitTestBase {
 protected:
  BatAdsTextClassificationProcessorTest() = default;

  ~BatAdsTextClassificationProcessorTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUp();

    resource_.Load();
    task_environment()->RunUntilIdle();
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
  const TextClassificationProbabilitiesList list =
      Client::Get()->GetTextClassificationProbabilitiesHistory();

  EXPECT_TRUE(list.empty());
}

TEST_F(BatAdsTextClassificationProcessorTest, DoNotProcessForEmptyText) {
  // Act
  const std::string text = "";
  processor::TextClassification processor(&resource_);
  processor.Process(text);

  // Assert
  const TextClassificationProbabilitiesList list =
      Client::Get()->GetTextClassificationProbabilitiesHistory();

  EXPECT_TRUE(list.empty());
}

TEST_F(BatAdsTextClassificationProcessorTest, NeverProcessed) {
  // Act
  model::TextClassification model;
  const SegmentList segments = model.GetSegments();

  // Assert
  const TextClassificationProbabilitiesList list =
      Client::Get()->GetTextClassificationProbabilitiesHistory();

  EXPECT_TRUE(list.empty());
}

TEST_F(BatAdsTextClassificationProcessorTest, ProcessText) {
  // Act
  const std::string text = "Some content about technology & computing";
  processor::TextClassification processor(&resource_);
  processor.Process(text);

  // Assert
  const TextClassificationProbabilitiesList list =
      Client::Get()->GetTextClassificationProbabilitiesHistory();

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
  const TextClassificationProbabilitiesList list =
      Client::Get()->GetTextClassificationProbabilitiesHistory();

  EXPECT_EQ(3UL, list.size());
}

}  // namespace ad_targeting
}  // namespace ads
