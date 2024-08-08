/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/model/text_classification_model.h"

#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_ads/core/internal/common/resources/language_components_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/resource/text_classification_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/text_classification_processor.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsTextClassificationModelTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    resource_ = std::make_unique<TextClassificationResource>();
  }

  std::unique_ptr<TextClassificationResource> resource_;
};

TEST_F(BraveAdsTextClassificationModelTest,
       DoNotGetSegmentsForUninitializedResource) {
  // Arrange
  TextClassificationProcessor processor(*resource_);
  processor.Process(/*text=*/"The quick brown fox jumps over the lazy dog");
  task_environment_.RunUntilIdle();

  // Act
  const SegmentList text_classification_segments =
      GetTextClassificationSegments();

  // Assert
  EXPECT_THAT(text_classification_segments, ::testing::IsEmpty());
}

TEST_F(BraveAdsTextClassificationModelTest, DoNotGetSegmentsForEmptyText) {
  // Arrange
  NotifyResourceComponentDidChange(test::kLanguageComponentManifestVersion,
                                   test::kLanguageComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  TextClassificationProcessor processor(*resource_);
  processor.Process(/*text=*/"");
  task_environment_.RunUntilIdle();

  // Act
  const SegmentList text_classification_segments =
      GetTextClassificationSegments();

  // Assert
  EXPECT_THAT(text_classification_segments, ::testing::IsEmpty());
}

TEST_F(BraveAdsTextClassificationModelTest,
       GetSegmentsForPreviouslyClassifiedText) {
  // Arrange
  NotifyResourceComponentDidChange(test::kLanguageComponentManifestVersion,
                                   test::kLanguageComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  TextClassificationProcessor processor(*resource_);
  processor.Process(/*text=*/"Some content about technology & computing");
  task_environment_.RunUntilIdle();

  // Act
  const SegmentList text_classification_segments =
      GetTextClassificationSegments();

  // Assert
  const SegmentList expected_text_classification_segments = {
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
  EXPECT_EQ(expected_text_classification_segments,
            text_classification_segments);
}

TEST_F(BraveAdsTextClassificationModelTest,
       GetSegmentsForPreviouslyClassifiedTexts) {
  // Arrange
  NotifyResourceComponentDidChange(test::kLanguageComponentManifestVersion,
                                   test::kLanguageComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  const std::vector<std::string> texts = {
      "Some content about cooking food", "Some content about finance & banking",
      "Some content about technology & computing"};

  TextClassificationProcessor processor(*resource_);
  for (const auto& text : texts) {
    processor.Process(text);
  }

  // Run the task environment until idle to ensure all tasks are processed.
  task_environment_.RunUntilIdle();

  // Act
  const SegmentList text_classification_segments =
      GetTextClassificationSegments();

  // Assert
  const SegmentList expected_text_classification_segments = {
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
  EXPECT_EQ(expected_text_classification_segments,
            text_classification_segments);
}

TEST_F(BraveAdsTextClassificationModelTest, DoNotGetSegmentsIfNeverProcessed) {
  // Arrange
  NotifyResourceComponentDidChange(test::kLanguageComponentManifestVersion,
                                   test::kLanguageComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  // Act
  const SegmentList text_classification_segments =
      GetTextClassificationSegments();

  // Assert
  EXPECT_THAT(text_classification_segments, ::testing::IsEmpty());
}

}  // namespace brave_ads
