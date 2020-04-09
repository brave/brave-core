/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <fstream>
#include <memory>
#include <string>
#include <sstream>

#include "base/files/file_path.h"
#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/page_classifier/page_classifier.h"
#include "bat/ads/internal/page_classifier/page_classifier_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "base/path_service.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

using ::testing::NiceMock;
using ::testing::Return;

namespace ads {

class BraveAdsPageClassifierTest : public ::testing::Test {
 protected:
  BraveAdsPageClassifierTest()
      : ads_client_mock_(std::make_unique<NiceMock<MockAdsClient>>()),
        ads_(std::make_unique<AdsImpl>(ads_client_mock_.get())) {
    // You can do set-up work for each test here

    InitializePageClassifier();
  }

  ~BraveAdsPageClassifierTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case

  base::FilePath GetResourcesPath() {
    base::FilePath path;
    base::PathService::Get(base::DIR_SOURCE_ROOT, &path);
    path = path.AppendASCII("brave/vendor/bat-native-ads/resources");
    return path;
  }

  bool Load(
      const base::FilePath path,
      std::string* value) {
    if (!value) {
      return false;
    }

    std::ifstream ifs{path.value().c_str()};
    if (ifs.fail()) {
      *value = "";
      return false;
    }

    std::stringstream stream;
    stream << ifs.rdbuf();
    *value = stream.str();
    return true;
  }

  void InitializePageClassifier() {
    page_classifier_ = std::make_unique<PageClassifier>(ads_.get());

    base::FilePath path = GetResourcesPath();
    path = path.AppendASCII("user_models");
    path = path.AppendASCII("languages");
    path = path.AppendASCII("en");
    path = path.AppendASCII("user_model.json");

    std::string json;
    ASSERT_TRUE(Load(path, &json));

    ASSERT_TRUE(page_classifier_->Initialize(json));
  }

  std::unique_ptr<MockAdsClient> ads_client_mock_;
  std::unique_ptr<AdsImpl> ads_;

  std::unique_ptr<PageClassifier> page_classifier_;
};

TEST_F(BraveAdsPageClassifierTest,
    ShouldClassifyPagesForUntargetedLocale) {
  // Arrange
  ON_CALL(*ads_client_mock_, GetLocale())
      .WillByDefault(Return("en-US"));

  const std::vector<std::string> user_model_languages = {
    "ja"
  };

  ON_CALL(*ads_client_mock_, GetUserModelLanguages())
      .WillByDefault(Return(user_model_languages));

  // Act
  const bool should_classify_pages = page_classifier_->ShouldClassifyPages();

  // Assert
  EXPECT_FALSE(should_classify_pages);
}

TEST_F(BraveAdsPageClassifierTest,
    ShouldClassifyPagesForTargetedLocale) {
  // Arrange
  ON_CALL(*ads_client_mock_, GetLocale())
      .WillByDefault(Return("en-US"));

  const std::vector<std::string> user_model_languages = {
    "en"
  };

  ON_CALL(*ads_client_mock_, GetUserModelLanguages())
      .WillByDefault(Return(user_model_languages));

  // Act
  const bool should_classify_pages = page_classifier_->ShouldClassifyPages();

  // Assert
  EXPECT_TRUE(should_classify_pages);
}

TEST_F(BraveAdsPageClassifierTest,
    ClassifyPageWithNoContent) {
  // Arrange
  const std::string content = "";

  // Act
  const std::string page_classification =
      page_classifier_->ClassifyPage("https://foobar.com", content);

  // Assert
  const std::string expected_page_classification = "";
  EXPECT_EQ(expected_page_classification, page_classification);
}

TEST_F(BraveAdsPageClassifierTest,
    ClassifyPageWithContent) {
  // Arrange
  const std::string content = "Some content about technology & computing";

  // Act
  const std::string page_classification =
      page_classifier_->ClassifyPage("https://foobar.com", content);

  // Assert
  const std::string expected_page_classification =
      "technology & computing-technology & computing";

  EXPECT_EQ(expected_page_classification, page_classification);
}

TEST_F(BraveAdsPageClassifierTest,
    GetWinningCategories) {
  // Arrange
  const std::vector<std::string> user_model_languages = {
    "en"
  };

  ON_CALL(*ads_client_mock_, GetUserModelLanguages())
      .WillByDefault(Return(user_model_languages));

  const std::vector<std::string> contents = {
    "Some content about cooking food",
    "Some content about finance & banking",
    "Some content about technology & computing"
  };

  for (const auto& content : contents) {
    page_classifier_->ClassifyPage("https://foobar.com", content);
  }

  // Act
  const CategoryList winning_categories =
      page_classifier_->GetWinningCategories();

  // Assert
  const CategoryList expected_winning_categories = {
    "technology & computing-technology & computing",
    "personal finance-banking",
    "food & drink-cooking"
  };

  EXPECT_EQ(expected_winning_categories, winning_categories);
}

TEST_F(BraveAdsPageClassifierTest,
    GetWinningCategoriesIfNoPagesHaveBeenClassified) {
  // Arrange
  const std::vector<std::string> user_model_languages = {
    "en"
  };

  ON_CALL(*ads_client_mock_, GetUserModelLanguages())
      .WillByDefault(Return(user_model_languages));

  // Act
  const CategoryList winning_categories =
      page_classifier_->GetWinningCategories();

  // Assert
  EXPECT_TRUE(winning_categories.empty());
}

TEST_F(BraveAdsPageClassifierTest,
    CachePageProbability) {
  // Arrange
  const std::string content = "Technology & computing content";
  const std::string page_classification =
      page_classifier_->ClassifyPage("https://foobar.com", content);

  // Act
  const PageProbabilitiesCacheMap page_probabilities_cache =
      page_classifier_->get_page_probabilities_cache();

  // Assert
  const int count = page_probabilities_cache.size();
  EXPECT_EQ(1, count);
}

TEST_F(BraveAdsPageClassifierTest,
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
