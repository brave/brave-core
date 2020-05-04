/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/page_classifier/page_classifier.h"

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/test/task_environment.h"
#include "base/path_service.h"
#include "brave/components/l10n/browser/locale_helper_mock.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/ads_unittest_utils.h"
#include "bat/ads/internal/page_classifier/page_classifier_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using ::testing::NiceMock;
using ::testing::Return;

namespace ads {

class BatAdsPageClassifierTest : public ::testing::Test {
 protected:
  BatAdsPageClassifierTest()
      : ads_client_mock_(std::make_unique<NiceMock<AdsClientMock>>()),
        ads_(std::make_unique<AdsImpl>(ads_client_mock_.get())),
        locale_helper_mock_(std::make_unique<NiceMock<
            brave_l10n::LocaleHelperMock>>()) {
    // You can do set-up work for each test here

    brave_l10n::LocaleHelper::GetInstance()->set_for_testing(
        locale_helper_mock_.get());
  }

  ~BatAdsPageClassifierTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)

    ON_CALL(*ads_client_mock_, IsEnabled())
        .WillByDefault(Return(true));

    ON_CALL(*locale_helper_mock_, GetLocale())
        .WillByDefault(Return("en-US"));

    MockLoad(ads_client_mock_.get());
    MockLoadUserModelForLanguage(ads_client_mock_.get());
    MockLoadJsonSchema(ads_client_mock_.get());
    MockSave(ads_client_mock_.get());

    Initialize(ads_.get());
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case

  base::test::TaskEnvironment task_environment_;

  std::unique_ptr<AdsClientMock> ads_client_mock_;
  std::unique_ptr<AdsImpl> ads_;
  std::unique_ptr<brave_l10n::LocaleHelperMock> locale_helper_mock_;
};

TEST_F(BatAdsPageClassifierTest,
    ShouldClassifyPagesForUntargetedLocale) {
  // Arrange
  ON_CALL(*locale_helper_mock_, GetLocale())
      .WillByDefault(Return("ja-JP"));

  // Act
  const bool should_classify_pages =
      ads_->get_page_classifier()->ShouldClassifyPages();

  // Assert
  EXPECT_FALSE(should_classify_pages);
}

TEST_F(BatAdsPageClassifierTest,
    ShouldClassifyPagesForTargetedLocale) {
  // Arrange

  // Act
  const bool should_classify_pages =
      ads_->get_page_classifier()->ShouldClassifyPages();

  // Assert
  EXPECT_TRUE(should_classify_pages);
}

TEST_F(BatAdsPageClassifierTest,
    ClassifyPageWithNoContent) {
  // Arrange
  const std::string content = "";

  // Act
  const std::string page_classification =
      ads_->get_page_classifier()->ClassifyPage("https://foobar.com", content);

  // Assert
  const std::string expected_page_classification = "";
  EXPECT_EQ(expected_page_classification, page_classification);
}

TEST_F(BatAdsPageClassifierTest,
    ClassifyPageWithContent) {
  // Arrange
  const std::string content = "Some content about technology & computing";

  // Act
  const std::string page_classification =
      ads_->get_page_classifier()->ClassifyPage("https://foobar.com", content);

  // Assert
  const std::string expected_page_classification =
      "technology & computing-technology & computing";

  EXPECT_EQ(expected_page_classification, page_classification);
}

TEST_F(BatAdsPageClassifierTest,
    GetWinningCategories) {
  // Arrange
  const std::vector<std::string> contents = {
    "Some content about cooking food",
    "Some content about finance & banking",
    "Some content about technology & computing"
  };

  for (const auto& content : contents) {
    ads_->get_page_classifier()->ClassifyPage("https://foobar.com", content);
  }

  // Act
  const CategoryList winning_categories =
      ads_->get_page_classifier()->GetWinningCategories();

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

  // Act
  const CategoryList winning_categories =
      ads_->get_page_classifier()->GetWinningCategories();

  // Assert
  EXPECT_TRUE(winning_categories.empty());
}

TEST_F(BatAdsPageClassifierTest,
    CachePageProbability) {
  // Arrange
  const std::string content = "Technology & computing content";
  const std::string page_classification =
      ads_->get_page_classifier()->ClassifyPage("https://foobar.com", content);

  // Act
  const PageProbabilitiesCacheMap page_probabilities_cache =
      ads_->get_page_classifier()->get_page_probabilities_cache();

  // Assert
  const int count = page_probabilities_cache.size();
  EXPECT_EQ(1, count);
}

TEST_F(BatAdsPageClassifierTest,
    NormalizeContent ) {
  // Arrange
  const std::string content =
      "  The quick brown fox jumps over the lazy dog. "
      "$123,000.0 !\"#$%&'()*+,-./:<=>?@\\[]^_`{|}~ 0123456789 \t\n\v\f\r "
      "0x7F x123x a1b2c3 Les naïfs ægithales hâtifs pondant à Noël où il "
      "gèle sont sûrs d'être déçus en voyant leurs drôles d'œufs abîmés. "
      "Falsches Üben von Xylophonmusik quält jeden größeren Zwerg. ξεσκεπάζω "
      "την ψυχοφθόρα \\t\\n\\v\\f\\r βδελυγμία. いろはにほへど　ちりぬるを "
      "わがよたれぞ　つねならむ うゐのおくやま　けふこえて あさきゆめみじ　"
      "ゑひもせず  ";  // The Quick Brown Fox... Pangrams

  // Act
  const std::string normalized_content =
      page_classifier::NormalizeContent(content);

  // Assert
  const std::string expected_normalized_content =
      "The quick brown fox jumps over the lazy dog Les naïfs ægithales hâtifs "
      "pondant à Noël où il gèle sont sûrs d être déçus en voyant leurs drôles "
      "d œufs abîmés Falsches Üben von Xylophonmusik quält jeden größeren "
      "Zwerg ξεσκεπάζω την ψυχοφθόρα βδελυγμία いろはにほへど　ちりぬるを "
      "わがよたれぞ　つねならむ うゐのおくやま　けふこえて あさきゆめみじ　"
      "ゑひもせず";

  EXPECT_EQ(expected_normalized_content, normalized_content);
}

}  // namespace ads
