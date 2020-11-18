/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/contextual/page_classifier/page_classifier.h"

#include "base/path_service.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace ad_targeting {
namespace contextual {

class BatAdsPageClassifierTest : public UnitTestBase {
 protected:
  BatAdsPageClassifierTest() = default;

  ~BatAdsPageClassifierTest() override = default;
};

TEST_F(BatAdsPageClassifierTest,
    DoNotClassifyPageForUntargetedLocale) {
  // Arrange
  PageClassifier page_classifier;
  page_classifier.LoadUserModelForLocale("ja-JP");

  const std::string content = "一部のコンテンツ";

  // Act
  const std::string page_classification =
      page_classifier.MaybeClassifyPage("https://foobar.com", content);

  // Assert
  const std::string expected_page_classification = "untargeted";

  EXPECT_EQ(expected_page_classification, page_classification);
}

TEST_F(BatAdsPageClassifierTest,
    ClassifyPageWithNoContent) {
  // Arrange
  PageClassifier page_classifier;
  page_classifier.LoadUserModelForLocale("en-US");

  const std::string content = "";

  // Act
  const std::string page_classification =
      page_classifier.MaybeClassifyPage("https://foobar.com", content);

  // Assert
  const std::string expected_page_classification = "";

  EXPECT_EQ(expected_page_classification, page_classification);
}

TEST_F(BatAdsPageClassifierTest,
    ClassifyPageWithContent) {
  // Arrange
  PageClassifier page_classifier;
  page_classifier.LoadUserModelForLocale("en-US");

  const std::string content = "Some content about technology & computing";

  // Act
  const std::string page_classification =
      page_classifier.MaybeClassifyPage("https://foobar.com", content);

  // Assert
  const std::string expected_page_classification =
      "technology & computing-technology & computing";

  EXPECT_EQ(expected_page_classification, page_classification);
}

TEST_F(BatAdsPageClassifierTest,
    GetWinningCategories) {
  // Arrange
  PageClassifier page_classifier;
  page_classifier.LoadUserModelForLocale("en-US");

  const std::vector<std::string> contents = {
    "Some content about cooking food",
    "Some content about finance & banking",
    "Some content about technology & computing"
  };

  for (const auto& content : contents) {
    page_classifier.MaybeClassifyPage("https://foobar.com", content);
  }

  // Act
  const CategoryList winning_categories =
      page_classifier.GetWinningCategories();

  // Assert
  const CategoryList expected_winning_categories = {
    "technology & computing-technology & computing",
    "personal finance-banking",
    "food & drink-cooking"
  };

  EXPECT_EQ(expected_winning_categories, winning_categories);
}

TEST_F(BatAdsPageClassifierTest,
    GetWinningCategoriesIfNoPagesHaveBeenClassified) {
  // Arrange
  PageClassifier page_classifier;
  page_classifier.LoadUserModelForLocale("en-US");

  // Act
  const CategoryList winning_categories =
      page_classifier.GetWinningCategories();

  // Assert
  EXPECT_TRUE(winning_categories.empty());
}

TEST_F(BatAdsPageClassifierTest,
    CachePageProbability) {
  // Arrange
  PageClassifier page_classifier;
  page_classifier.LoadUserModelForLocale("en-US");

  const std::string content = "Technology & computing content";

  const std::string page_classification =
      page_classifier.MaybeClassifyPage("https://foobar.com", content);

  // Act
  const PageProbabilitiesCacheMap page_probabilities_cache
      = page_classifier.get_page_probabilities_cache();

  // Assert
  const int count = page_probabilities_cache.size();
  EXPECT_EQ(1, count);
}

}  // namespace contextual
}  // namespace ad_targeting
}  // namespace ads
