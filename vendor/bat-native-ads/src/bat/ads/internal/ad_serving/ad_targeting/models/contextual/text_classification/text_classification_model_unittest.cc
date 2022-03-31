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
namespace model {

class BatAdsTextClassificationModelTest : public UnitTestBase {
 protected:
  BatAdsTextClassificationModelTest() = default;

  ~BatAdsTextClassificationModelTest() override = default;
};

TEST_F(BatAdsTextClassificationModelTest,
       DoNotGetSegmentsForUninitializedResource) {
  // Arrange
  resource::TextClassification resource;

  const std::string text = "The quick brown fox jumps over the lazy dog";
  processor::TextClassification processor(&resource);
  processor.Process(text);

  // Act
  TextClassification model;
  const SegmentList segments = model.GetSegments();

  // Assert
  const SegmentList expected_segments = {};

  EXPECT_EQ(expected_segments, segments);
}

TEST_F(BatAdsTextClassificationModelTest, DoNotGetSegmentsForEmptyText) {
  // Arrange
  resource::TextClassification resource;
  resource.Load();

  const std::string text = "";
  processor::TextClassification processor(&resource);
  processor.Process(text);

  // Act
  TextClassification model;
  const SegmentList segments = model.GetSegments();

  // Assert
  const SegmentList expected_segments = {};

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
  TextClassification model;
  const SegmentList segments = model.GetSegments();

  // Assert
  const SegmentList expected_segments = {
      "technology & computing-technology & computing",
      "technology & computing-unix",
      "science-geology",
      "technology & computing-software",
      "sports-fishing",
      "sports-swimming",
      "law-law",
      "sports-american football",
      "health & fitness-sex",
      "sports-golf",
      "home-appliances",
      "history-history",
      "personal finance-banking",
      "home-interior design",
      "technology & computing-apple",
      "personal finance-tax",
      "folklore-astrology",
      "arts & entertainment-literature",
      "food & drink-vegetarian",
      "history-archaeology",
      "personal finance-investing",
      "sports-snowboarding",
      "pets-pets",
      "food & drink-wine",
      "business-marketing",
      "gaming-gaming",
      "sports-cycling",
      "sports-volleyball",
      "fashion-jewelry",
      "real estate-real estate",
      "science-mathematics",
      "health & fitness-bodybuilding",
      "science-science",
      "hobbies & interests-photography",
      "pets-aquariums",
      "arts & entertainment-anime",
      "automotive-pickup trucks",
      "business-business",
      "folklore-paranormal phenomena",
      "technology & computing-programming",
      "fashion-fashion",
      "sports-archery",
      "sports-surfing",
      "arts & entertainment-film",
      "food & drink-baking",
      "fashion-clothing",
      "education-education",
      "hobbies & interests-coins",
      "pets-birds",
      "food & drink-cocktails"};

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
  TextClassification model;
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
      "science-economics",
      "sports-american football",
      "home-appliances",
      "sports-swimming",
      "sports-fishing",
      "pets-aquariums",
      "hobbies & interests-coins",
      "fashion-clothing",
      "technology & computing-apple",
      "pets-pets",
      "gaming-gaming",
      "history-archaeology",
      "folklore-astrology",
      "sports-cycling",
      "pets-birds",
      "business-marketing",
      "personal finance-tax",
      "sports-sports",
      "technology & computing-software",
      "sports-surfing",
      "real estate-real estate",
      "sports-archery",
      "food & drink-food & drink",
      "arts & entertainment-television",
      "arts & entertainment-film",
      "science-biology",
      "sports-basketball",
      "arts & entertainment-literature",
      "science-science",
      "food & drink-baking",
      "sports-skateboarding",
      "arts & entertainment-anime",
      "business-business",
      "history-history",
      "sports-skiing",
      "home-interior design",
      "sports-tennis",
      "food & drink-wine",
      "hobbies & interests-horse racing",
      "fashion-jewelry",
      "folklore-paranormal phenomena",
      "arts & entertainment-animation",
      "technology & computing-programming",
      "sports-boxing",
      "fashion-fashion",
      "hobbies & interests-needlework",
      "sports-climbing",
      "food & drink-cocktails",
      "health & fitness-diet & nutrition",
      "hobbies & interests-smoking",
      "law-law",
      "sports-jogging",
      "food & drink-barbecues & grilling",
      "health & fitness-sex",
      "sports-golf",
      "business-energy",
      "personal finance-investing",
      "sports-snowboarding",
      "science-astronomy",
      "personal finance-credit & debt & loans",
      "travel-adventure travel",
      "arts & entertainment-poetry",
      "science-geography",
      "sports-volleyball",
      "health & fitness-dental care",
      "science-mathematics",
      "health & fitness-bodybuilding",
      "automotive-automotive",
      "hobbies & interests-photography",
      "science-palaeontology",
      "other-other",
      "home-garden",
      "automotive-pickup trucks",
      "home-home",
      "personal finance-insurance",
      "travel-air travel",
      "hobbies & interests-arts & crafts",
      "sports-athletics",
      "technology & computing-database",
      "pets-dogs",
      "travel-hotels",
      "technology & computing-freeware",
      "architecture-architecture",
      "hobbies & interests-hobbies & interests",
      "arts & entertainment-arts & entertainment",
      "education-education",
      "hobbies & interests-dance"};

  EXPECT_EQ(expected_segments, segments);
}

TEST_F(BatAdsTextClassificationModelTest, DoNotGetSegmentsIfNeverProcessed) {
  // Arrange
  resource::TextClassification resource;
  resource.Load();

  // Act
  TextClassification model;
  const SegmentList segments = model.GetSegments();

  // Assert
  const SegmentList expected_segments = {};

  EXPECT_EQ(expected_segments, segments);
}

}  // namespace model
}  // namespace ad_targeting
}  // namespace ads
