/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/contextual/page_classifier/page_classifier.h"

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "base/test/task_environment.h"
#include "brave/components/l10n/browser/locale_helper_mock.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/platform/platform_helper_mock.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using ::testing::NiceMock;
using ::testing::Return;

namespace ads {
namespace ad_targeting {
namespace contextual {

class BatAdsPageClassifierTest : public ::testing::Test {
 protected:
  BatAdsPageClassifierTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        ads_client_mock_(std::make_unique<NiceMock<AdsClientMock>>()),
        ads_(std::make_unique<AdsImpl>(ads_client_mock_.get())),
        locale_helper_mock_(std::make_unique<
            NiceMock<brave_l10n::LocaleHelperMock>>()),
        platform_helper_mock_(std::make_unique<
            NiceMock<PlatformHelperMock>>()) {
    // You can do set-up work for each test here

    brave_l10n::LocaleHelper::GetInstance()->set_for_testing(
        locale_helper_mock_.get());

    PlatformHelper::GetInstance()->set_for_testing(platform_helper_mock_.get());
  }

  ~BatAdsPageClassifierTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)

    const std::string locale = "en-US";

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    const base::FilePath path = temp_dir_.GetPath();

    SetBuildChannel(false, "test");

    ON_CALL(*locale_helper_mock_, GetLocale())
        .WillByDefault(Return(locale));

    MockPlatformHelper(platform_helper_mock_, PlatformType::kMacOS);

    ads_->OnWalletUpdated("c387c2d8-a26d-4451-83e4-5c0c6fd942be",
        "5BEKM1Y7xcRSg/1q8in/+Lki2weFZQB+UMYZlRw8ql8=");

    MockLoad(ads_client_mock_);
    MockLoadUserModelForId(ads_client_mock_);
    MockLoadResourceForId(ads_client_mock_);
    MockSave(ads_client_mock_);

    MockPrefs(ads_client_mock_);

    database_ = std::make_unique<Database>(path.AppendASCII("database.sqlite"));
    MockRunDBTransaction(ads_client_mock_, database_);

    Initialize(ads_);

    ads_->ChangeLocale("en-US");
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case

  PageClassifier* get_page_classifier() {
    return ads_->get_page_classifier();
  }

  base::test::TaskEnvironment task_environment_;

  base::ScopedTempDir temp_dir_;

  std::unique_ptr<AdsClientMock> ads_client_mock_;
  std::unique_ptr<AdsImpl> ads_;
  std::unique_ptr<brave_l10n::LocaleHelperMock> locale_helper_mock_;
  std::unique_ptr<PlatformHelperMock> platform_helper_mock_;
  std::unique_ptr<Database> database_;
};

TEST_F(BatAdsPageClassifierTest,
    DoNotClassifyPageForUntargetedLocale) {
  // Arrange
  const std::string locale = "ja-JP";

  ON_CALL(*locale_helper_mock_, GetLocale())
      .WillByDefault(Return(locale));

  ads_->ChangeLocale(locale);

  const std::string content = "一部のコンテンツ";

  // Act
  const std::string page_classification =
      get_page_classifier()->MaybeClassifyPage("https://foobar.com", content);

  // Assert
  const std::string expected_page_classification = "untargeted";

  EXPECT_EQ(expected_page_classification, page_classification);
}

TEST_F(BatAdsPageClassifierTest,
    ClassifyPageWithNoContent) {
  // Arrange
  const std::string content = "";

  // Act
  const std::string page_classification =
      get_page_classifier()->MaybeClassifyPage("https://foobar.com", content);

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
      get_page_classifier()->MaybeClassifyPage("https://foobar.com", content);

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
    get_page_classifier()->MaybeClassifyPage("https://foobar.com", content);
  }

  // Act
  const CategoryList winning_categories =
      get_page_classifier()->GetWinningCategories();

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
      get_page_classifier()->GetWinningCategories();

  // Assert
  EXPECT_TRUE(winning_categories.empty());
}

TEST_F(BatAdsPageClassifierTest,
    CachePageProbability) {
  // Arrange
  const std::string content = "Technology & computing content";
  const std::string page_classification =
      get_page_classifier()->MaybeClassifyPage("https://foobar.com", content);

  // Act
  const PageProbabilitiesCacheMap page_probabilities_cache =
      get_page_classifier()->get_page_probabilities_cache();

  // Assert
  const int count = page_probabilities_cache.size();
  EXPECT_EQ(1, count);
}

}  // namespace contextual
}  // namespace ad_targeting
}  // namespace ads
