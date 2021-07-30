/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_serving/ad_targeting/models/contextual/text_classification/text_classification_model.h"

#include <string>
#include <vector>

#include "bat/ads/internal/ad_targeting/processors/contextual/text_classification/text_classification_processor.h"
#include "bat/ads/internal/resources/contextual/text_classification/text_classification_resource.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace ad_targeting {

class BatAdsTextClassificationModelTest : public UnitTestBase {
 protected:
  BatAdsTextClassificationModelTest() = default;

  ~BatAdsTextClassificationModelTest() override = default;
};

TEST_F(BatAdsTextClassificationModelTest,
       GetUntargetedSegmentForUninitializedResource) {
  // Arrange
  resource::TextClassification resource;

  const std::string text = "The quick brown fox jumps over the lazy dog";
  processor::TextClassification processor(&resource);
  processor.Process(text);

  // Act
  model::TextClassification model;
  const SegmentList segments = model.GetSegments();

  // Assert
  const SegmentList expected_segments = {"untargeted"};

  EXPECT_EQ(expected_segments, segments);
}

TEST_F(BatAdsTextClassificationModelTest, GetUntargetedSegmentForEmptyText) {
  // Arrange
  resource::TextClassification resource;
  resource.Load();

  const std::string text = "";
  processor::TextClassification processor(&resource);
  processor.Process(text);

  // Act
  model::TextClassification model;
  const SegmentList segments = model.GetSegments();

  // Assert
  const SegmentList expected_segments = {"untargeted"};

  EXPECT_EQ(expected_segments, segments);
}

TEST_F(BatAdsTextClassificationModelTest,
       GetSegmentsForPreviouslyClassifiedText) {
  // Arrange
  resource::TextClassification resource;
  resource.Load();

  const std::string text = "Some content about technology & computing";
  processor::TextClassification processor(&resource);
  processor.Process(text);

  // Act
  model::TextClassification model;
  const SegmentList segments = model.GetSegments();

  // Assert
  const SegmentList expected_segments = {
      "technology & computing-technology & computing",
      "technology & computing-unix", "science-geology"};

  EXPECT_EQ(expected_segments, segments);
}

TEST_F(BatAdsTextClassificationModelTest,
       GetSegmentsForPreviouslyClassifiedTexts) {
  // Arrange
  resource::TextClassification resource;
  resource.Load();

  const std::vector<std::string> texts = {
      "Some content about cooking food", "Some content about finance & banking",
      "Some content about technology & computing"};

  processor::TextClassification processor(&resource);
  for (const auto& text : texts) {
    processor.Process(text);
  }

  // Act
  model::TextClassification model;
  const SegmentList segments = model.GetSegments();

  // Assert
  const SegmentList expected_segments = {
      "technology & computing-technology & computing",
      "personal finance-banking", "food & drink-cooking"};

  EXPECT_EQ(expected_segments, segments);
}

TEST_F(BatAdsTextClassificationModelTest,
       GetUntargetedSegmentIfNeverProcessed) {
  // Arrange
  resource::TextClassification resource;
  resource.Load();

  // Act
  model::TextClassification model;
  const SegmentList segments = model.GetSegments();

  // Assert
  const SegmentList expected_segments = {"untargeted"};

  EXPECT_EQ(expected_segments, segments);
}

}  // namespace ad_targeting
}  // namespace ads
