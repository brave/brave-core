/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/targeting/contextual/text_classification/text_classification_model.h"

#include <string>
#include <vector>

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/processors/contextual/text_classification/text_classification_processor.h"
#include "brave/components/brave_ads/core/internal/resources/contextual/text_classification/text_classification_resource.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::targeting::model {

class BraveAdsTextClassificationModelTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    resource_.Load();
    task_environment_.RunUntilIdle();
  }

  resource::TextClassification resource_;
};

TEST_F(BraveAdsTextClassificationModelTest,
       DoNotGetSegmentsForUninitializedResource) {
  // Arrange
  resource::TextClassification resource;

  processor::TextClassification processor(resource);
  processor.Process(/*text*/ "The quick brown fox jumps over the lazy dog");

  const TextClassification model;

  // Act
  const SegmentList segments = model.GetSegments();

  // Assert
  EXPECT_TRUE(segments.empty());
}

TEST_F(BraveAdsTextClassificationModelTest, DoNotGetSegmentsForEmptyText) {
  // Arrange
  const std::string text;
  processor::TextClassification processor(resource_);
  processor.Process(text);

  const TextClassification model;

  // Act
  const SegmentList segments = model.GetSegments();

  // Assert
  EXPECT_TRUE(segments.empty());
}

TEST_F(BraveAdsTextClassificationModelTest,
       GetSegmentsForPreviouslyClassifiedText) {
  // Arrange
  processor::TextClassification processor(resource_);
  processor.Process(/*text*/ "Some content about technology & computing");

  const TextClassification model;

  // Act
  const SegmentList segments = model.GetSegments();

  // Assert
  const SegmentList expected_segments = {
      "technology & computing-technology & computing",
      "technology & computing-unix",
      "science-geology",
      "sports-american football",
      "technology & computing-software",
      "sports-fishing",
      "sports-swimming",
      "health & fitness-sex",
      "personal finance-banking",
      "sports-golf",
      "home-appliances",
      "personal finance-investing",
      "hobbies & interests-genealogy",
      "folklore-astrology",
      "sports-cycling",
      "law-law",
      "sports-volleyball",
      "history-archaeology",
      "technology & computing-programming",
      "health & fitness-bodybuilding",
      "sports-snowboarding",
      "personal finance-tax",
      "home-interior design",
      "technology & computing-apple",
      "hobbies & interests-photography",
      "automotive-pickup trucks",
      "arts & entertainment-literature",
      "history-history",
      "arts & entertainment-anime",
      "food & drink-vegetarian",
      "pets-pets",
      "arts & entertainment-film",
      "business-business",
      "sports-skiing",
      "business-marketing",
      "education-education",
      "science-mathematics",
      "gaming-gaming",
      "sports-surfing",
      "pets-aquariums",
      "sports-archery",
      "food & drink-cocktails",
      "fashion-jewelry",
      "fashion-clothing",
      "fashion-fashion",
      "food & drink-baking",
      "real estate-real estate",
      "hobbies & interests-coins",
      "food & drink-vegan",
      "food & drink-wine",
      "sports-athletics",
      "pets-birds",
      "food & drink-food & drink",
      "science-science",
      "arts & entertainment-animation",
      "personal finance-insurance"};

  EXPECT_EQ(expected_segments, segments);
}

TEST_F(BraveAdsTextClassificationModelTest,
       GetSegmentsForPreviouslyClassifiedTexts) {
  // Arrange
  const std::vector<std::string> texts = {
      "Some content about cooking food", "Some content about finance & banking",
      "Some content about technology & computing"};

  processor::TextClassification processor(resource_);
  for (const auto& text : texts) {
    processor.Process(text);
  }

  const TextClassification model;

  // Act
  const SegmentList segments = model.GetSegments();

  // Assert
  const SegmentList expected_segments = {
      "technology & computing-technology & computing",
      "personal finance-banking",
      "food & drink-cooking",
      "science-geology",
      "technology & computing-unix",
      "personal finance-personal finance",
      "food & drink-vegetarian",
      "sports-american football",
      "science-economics",
      "food & drink-food & drink",
      "sports-fishing",
      "sports-swimming",
      "pets-aquariums",
      "hobbies & interests-coins",
      "gaming-gaming",
      "technology & computing-apple",
      "folklore-astrology",
      "history-archaeology",
      "pets-pets",
      "pets-birds",
      "technology & computing-software",
      "sports-surfing",
      "sports-skiing",
      "sports-cycling",
      "business-marketing",
      "arts & entertainment-animation",
      "sports-sports",
      "sports-archery",
      "arts & entertainment-film",
      "food & drink-wine",
      "home-appliances",
      "health & fitness-sex",
      "fashion-clothing",
      "sports-basketball",
      "arts & entertainment-anime",
      "science-biology",
      "business-business",
      "food & drink-baking",
      "food & drink-barbecues & grilling",
      "sports-skateboarding",
      "science-science",
      "arts & entertainment-literature",
      "technology & computing-programming",
      "hobbies & interests-horse racing",
      "personal finance-tax",
      "home-interior design",
      "sports-tennis",
      "history-history",
      "hobbies & interests-needlework",
      "real estate-real estate",
      "food & drink-cocktails",
      "sports-boxing",
      "fashion-jewelry",
      "sports-climbing",
      "fashion-fashion",
      "personal finance-insurance",
      "arts & entertainment-television",
      "health & fitness-diet & nutrition",
      "hobbies & interests-smoking",
      "sports-jogging",
      "sports-golf",
      "personal finance-credit & debt & loans",
      "personal finance-investing",
      "hobbies & interests-genealogy",
      "business-energy",
      "law-law",
      "sports-volleyball",
      "health & fitness-bodybuilding",
      "sports-snowboarding",
      "science-astronomy",
      "hobbies & interests-photography",
      "automotive-pickup trucks",
      "arts & entertainment-poetry",
      "science-geography",
      "health & fitness-dental care",
      "science-palaeontology",
      "other-other",
      "education-education",
      "science-mathematics",
      "home-garden",
      "home-home",
      "folklore-paranormal phenomena",
      "travel-air travel",
      "hobbies & interests-hobbies & interests",
      "food & drink-vegan",
      "pets-dogs",
      "travel-hotels",
      "technology & computing-freeware",
      "sports-cricket",
      "hobbies & interests-arts & crafts",
      "architecture-architecture",
      "sports-athletics",
      "health & fitness-exercise",
      "arts & entertainment-arts & entertainment",
      "hobbies & interests-dance",
      "travel-adventure travel",
      "food & drink-pasta"};

  EXPECT_EQ(expected_segments, segments);
}

TEST_F(BraveAdsTextClassificationModelTest, DoNotGetSegmentsIfNeverProcessed) {
  // Arrange
  const TextClassification model;

  // Act
  const SegmentList segments = model.GetSegments();

  // Assert
  EXPECT_TRUE(segments.empty());
}

}  // namespace brave_ads::targeting::model
