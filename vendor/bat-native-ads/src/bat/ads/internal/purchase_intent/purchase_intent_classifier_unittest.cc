/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdint.h>
#include <memory>
#include <tuple>

#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/ads_impl.h"

#include "bat/ads/internal/time.h"
#include "base/time/time.h"

#include "testing/gtest/include/gtest/gtest.h"

#include "bat/ads/internal/static_values.h"
#include "bat/ads/internal/purchase_intent/purchase_intent_classifier.h"
#include "bat/ads/internal/purchase_intent/funnel_sites.h"

using ::testing::_;

// npm run test -- brave_unit_tests --filter=AdsPurchaseIntentClassifier*

namespace ads {

const std::vector<std::string> audi_a6_segments = {
  "automotive purchase intent by make-audi",
  "automotive purchase intent by category-mid luxury car"
};

const std::vector<std::string> no_segments;

struct TestTriplets {
  std::string url;
  std::vector<std::string> segments;
  uint16_t weight;
};

std::vector<TestTriplets> kTestSearchQueries = {
  {"https://yandex.com/search/?text=audi%20a6%20review%202020&lr=109565",
      audi_a6_segments, 2},
  {"https://www.google.com/search?q=audi+a6+review+2020&gs_l=psy-ab.3..0j0i22i30l3.26031.26031..26262...0.0..0.82.82.1......0....2j1..gws-wiz.MZlXqcvydls&ved=0ahUKEwjAjpziu8fnAhVLzYUKHSriDZMQ4dUDCAg&uact=5",  // NOLINT
      audi_a6_segments, 2},
  {"https://www.google.com/search?q=audi+a6+review+2020&oq=audi&aqs=chrome.1.69i59l2j69i57j69i60l3.2273j0j1&sourceid=chrome&ie=UTF-8",  // NOLINT
      audi_a6_segments, 2},
  {"https://www.bing.com/search?q=audi+a6+review+2020&qs=HS&pq=audi+a&sc=8-6&cvid=68F9883A6926440F8F6CCCBCDB87A7AA&FORM=QBLH&sp=1",   // NOLINT
      audi_a6_segments, 2},
  {"https://duckduckgo.com/?q=audi+a6+dealer+reviews&t=h_&ia=web",
      audi_a6_segments, 3},
  {"https://duckduckgo.com/?q=audi+a6&t=h_&ia=web",
      audi_a6_segments, 1},
  {"https://search.yahoo.com/search?p=audi+a6+review+2020&fr=sfp&iscqry=",
      audi_a6_segments, 2},
  {"https://fireball.com/search?q=audi+a6+review",
      audi_a6_segments, 2},
  {"https://www.ecosia.org/search?q=audi+a6+review",
      audi_a6_segments, 2},
  {"https://www.kbb.com/bmw/6-series/2017/styles/?intent=trade-in-sell&mileage=100000",  // NOLINT
      _funnel_site_segments, 1},
  {"https://www.cars.com/for-sale/searchresults.action/?mkId=20050&rd=10&searchSource=QUICK_FORM&zc=10001",  // NOLINT
      _funnel_site_segments, 1},
  {"https://www.google.com/search?source=hp&ei=lY5BXpenN-qUlwSd7bvICQ&q=foo+bar&oq=foo+bar&gs_l=psy-ab.3..0l10.1452.2016..2109...0.0..0.57.381.7......0....1..gws-wiz.......0i131j0i10.CeBo7A4BiSM&ved=0ahUKEwjXxbuPvcfnAhVqyoUKHZ32DpkQ4dUDCAg&uact=5",  // NOLINT
      no_segments, 0},
  {"https://creators.brave.com/",
      no_segments, 0},
};

class AdsPurchaseIntentClassifierTest : public ::testing::Test {
 protected:
  std::unique_ptr<MockAdsClient> mock_ads_client_;
  std::unique_ptr<AdsImpl> ads_;

  AdsPurchaseIntentClassifierTest() :
      mock_ads_client_(std::make_unique<MockAdsClient>()),
      ads_(std::make_unique<AdsImpl>(mock_ads_client_.get())) {
    // You can do set-up work for each test here
  }

  ~AdsPurchaseIntentClassifierTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)
    purchase_intent_classifier_ = std::make_unique<PurchaseIntentClassifier>(
        kPurchaseIntentSignalLevel, kPurchaseIntentClassificationThreshold,
            kPurchaseIntentSignalDecayTimeWindow);

    auto now = Time::NowInSeconds();
    auto days = base::Time::kSecondsPerHour * base::Time::kHoursPerDay;

    auto p1 = PurchaseIntentSignalHistory();
    p1.timestamp_in_seconds = now - (6 * days);
    p1.weight = 9;
    auto p2 = PurchaseIntentSignalHistory();
    p2.timestamp_in_seconds = now - (5 * days);
    p2.weight = 9;
    auto p3 = PurchaseIntentSignalHistory();
    p3.timestamp_in_seconds = now - (4 * days);
    p3.weight = 9;
    auto p4 = PurchaseIntentSignalHistory();
    p4.timestamp_in_seconds = now - (3 * days);
    p4.weight = 1;
    auto p5 = PurchaseIntentSignalHistory();
    p5.timestamp_in_seconds = now - (2 * days);
    p5.weight = 1;
    auto p6 = PurchaseIntentSignalHistory();
    p6.timestamp_in_seconds = now - (1 * days);
    p6.weight = 2;
    auto p7 = PurchaseIntentSignalHistory();
    p7.timestamp_in_seconds = now - (1 * days);
    p7.weight = 1;
    auto p8 = PurchaseIntentSignalHistory();
    p8.timestamp_in_seconds = now - (1 * days);
    p8.weight = 1;
    auto p9 = PurchaseIntentSignalHistory();
    p9.timestamp_in_seconds = now - (1 * days);
    p9.weight = 1;
    auto p10 = PurchaseIntentSignalHistory();
    p10.timestamp_in_seconds = now - (1 * days);
    p10.weight = 1;
    auto p11 = PurchaseIntentSignalHistory();
    p11.timestamp_in_seconds = now - (1 * days);
    p11.weight = 1;
    auto p12 = PurchaseIntentSignalHistory();
    p12.timestamp_in_seconds = now - (1 * days);
    p12.weight = 8;

    histories_ = {
      {"cat_5", {p1, p4, p8, p9, p10}},  // Score: 13
      {"cat_2", {p2, p5, p8, p11}},  // Score: 12
      {"cat_1", {p3, p6, p8}},  // Score: 12
      {"cat_4", {p7}},  // Score: 1
      {"cat_3", {p8}},  // Score: 1
      {"cat_6", {p6, p12}},  // Score: 10
      {"cat_7", {p6, p10, p12}}  // Score: 11
    };

    histories_short_ = {
      {"cat_1", {p1, p4}},
      {"cat_2", {p2, p3}}
    };
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case
  std::unique_ptr<PurchaseIntentClassifier> purchase_intent_classifier_;
  PurchaseIntentSignalSegmentHistoryMap histories_;
  PurchaseIntentSignalSegmentHistoryMap histories_short_;
  PurchaseIntentSignalSegmentHistoryMap histories_empty_;
};

TEST_F(AdsPurchaseIntentClassifierTest, ExtractsPurchaseIntentSignal) {
  for (const auto& test_search_query : kTestSearchQueries) {
    // Arrange
    auto url = test_search_query.url;
    auto segments = test_search_query.segments;
    auto weight = test_search_query.weight;

    // Act
    PurchaseIntentSignalInfo purchase_intent_signal =
        purchase_intent_classifier_->ExtractIntentSignal(url);

    // Assert
    EXPECT_EQ(segments, purchase_intent_signal.segments);
    EXPECT_EQ(weight, purchase_intent_signal.weight);
  }
}

TEST_F(AdsPurchaseIntentClassifierTest, GetsWinningCategoriesWithEmptyHistory) {
  // Arrange

  // Act
  auto winning_categories = purchase_intent_classifier_->GetWinningCategories(
      histories_empty_, 3);

  // Assert
  std::vector<std::string> gold_categories;
  EXPECT_EQ(gold_categories, winning_categories);
}

TEST_F(AdsPurchaseIntentClassifierTest, GetsWinningCategoriesWithShortHistory) {
  // Arrange
  std::map<std::string, std::deque<PurchaseIntentSignalHistory>> histories;

  // Act
  auto winning_categories = purchase_intent_classifier_->GetWinningCategories(
      histories_short_, 3);

  // Assert
  std::vector<std::string> gold_categories = {"cat_2"};
  EXPECT_EQ(gold_categories, winning_categories);
}

TEST_F(AdsPurchaseIntentClassifierTest, GetsWinningCategories) {
  // Arrange

  // Act
  auto winning_categories = purchase_intent_classifier_->GetWinningCategories(
      histories_, 5);

  // Assert
  std::vector<std::string> gold_categories =
      {"cat_5", "cat_2", "cat_1", "cat_7"};
  EXPECT_EQ(gold_categories, winning_categories);
}

TEST_F(AdsPurchaseIntentClassifierTest, GetsWinningCategoriesUpToMaxSegments) {
  // Arrange

  // Act
  auto winning_categories = purchase_intent_classifier_->GetWinningCategories(
      histories_, 2);

  // Assert
  std::vector<std::string> gold_categories =
      {"cat_5", "cat_2"};
  EXPECT_EQ(gold_categories, winning_categories);
}

}  // namespace ads
