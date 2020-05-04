/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/purchase_intent/purchase_intent_classifier.h"

#include <stdint.h>

#include <deque>
#include <map>
#include <memory>
#include <vector>

#include "base/time/time.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "bat/ads/internal/purchase_intent/funnel_sites.h"
#include "bat/ads/internal/static_values.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

const int64_t kSecondsPerDay =
    base::Time::kHoursPerDay * base::Time::kSecondsPerHour;

const std::vector<std::string> kAudiA4Segments = {
  "automotive purchase intent by make-audi",
  "automotive purchase intent by category-entry luxury car"
};

const std::vector<std::string> kAudiA6Segments = {
  "automotive purchase intent by make-audi",
  "automotive purchase intent by category-mid luxury car"
};

const std::vector<std::string> kNoSegments;

struct TestTriplets {
  std::string url;
  std::vector<std::string> segments;
  uint16_t weight;
};

std::vector<TestTriplets> kTestSearchQueries = {
  {
    "https://yandex.com/search/?text=audi%20a6%20review%202020&lr=109565",
    kAudiA6Segments,
    2
  },
  {
    "https://www.google.com/search?q=audi+a6+review+2020&gs_l=psy-ab.3..0j0i22i30l3.26031.26031..26262...0.0..0.82.82.1......0....2j1..gws-wiz.MZlXqcvydls&ved=0ahUKEwjAjpziu8fnAhVLzYUKHSriDZMQ4dUDCAg&uact=5",  // NOLINT
    kAudiA6Segments,
    2
  },
  {
    "https://www.google.com/search?q=audi+a6+review+2020&oq=audi&aqs=chrome.1.69i59l2j69i57j69i60l3.2273j0j1&sourceid=chrome&ie=UTF-8",  // NOLINT
    kAudiA6Segments,
    2
  },
  {
    "https://www.bing.com/search?q=audi+a6+review+2020&qs=HS&pq=audi+a&sc=8-6&cvid=68F9883A6926440F8F6CCCBCDB87A7AA&FORM=QBLH&sp=1",  // NOLINT
    kAudiA6Segments,
    2
  },
  {
    "https://duckduckgo.com/?q=audi+a6+dealer+reviews&t=h_&ia=web",
    kAudiA6Segments,
    3
  },
  {
    "https://duckduckgo.com/?q=audi+a6&t=h_&ia=web",
    kAudiA6Segments,
    1
  },
  {
    "https://search.yahoo.com/search?p=audi+a6+review+2020&fr=sfp&iscqry=",
    kAudiA6Segments,
    2
  },
  {
    "https://fireball.com/search?q=audi+a6+review",
    kAudiA6Segments,
    2
  },
  {
    "https://www.ecosia.org/search?q=audi+a6+review",
    kAudiA6Segments,
    2
  },
  {
    "https://www.kbb.com/bmw/6-series/2017/styles/?intent=trade-in-sell&mileage=100000",  // NOLINT
    _funnel_site_segments,
    1
  },
  {
    "https://www.cars.com/for-sale/searchresults.action/?mkId=20050&rd=10&searchSource=QUICK_FORM&zc=10001",  // NOLINT
    _funnel_site_segments,
    1
  },
  {
    "https://www.google.com/search?source=hp&ei=lY5BXpenN-qUlwSd7bvICQ&q=foo+bar&oq=foo+bar&gs_l=psy-ab.3..0l10.1452.2016..2109...0.0..0.57.381.7......0....1..gws-wiz.......0i131j0i10.CeBo7A4BiSM&ved=0ahUKEwjXxbuPvcfnAhVqyoUKHZ32DpkQ4dUDCAg&uact=5",  // NOLINT
    kNoSegments,
    0
  },
  {
    "https://creators.brave.com/",
    kNoSegments,
    0
  },
  {
    "https://www.google.com/search?ei=bY2CXvHBMK2tytMPqYq--A4&q=audi+a4",
    kAudiA4Segments,
    1
  },
  {
    "https://www.google.com/search?source=hp&ei=y2eDXsWuI6fIrgThwZuQDw&q=audi+a4",  // NOLINT
    kAudiA4Segments,
    1
  }
};

}  // namespace

class AdsPurchaseIntentClassifierTest : public ::testing::Test {
 protected:
  AdsPurchaseIntentClassifierTest()
      : purchase_intent_classifier_(std::make_unique<
            PurchaseIntentClassifier>(kPurchaseIntentSignalLevel,
                kPurchaseIntentClassificationThreshold,
                    kPurchaseIntentSignalDecayTimeWindow)) {
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
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case

  PurchaseIntentSignalSegmentHistoryMap GenerateHistory() {
    const uint64_t now_in_seconds =
        static_cast<uint64_t>(base::Time::Now().ToDoubleT());

    PurchaseIntentSignalHistory p1;
    p1.timestamp_in_seconds = now_in_seconds - (6 * kSecondsPerDay);
    p1.weight = 9;

    PurchaseIntentSignalHistory p2;
    p2.timestamp_in_seconds = now_in_seconds - (5 * kSecondsPerDay);
    p2.weight = 9;

    PurchaseIntentSignalHistory p3;
    p3.timestamp_in_seconds = now_in_seconds - (4 * kSecondsPerDay);
    p3.weight = 9;

    PurchaseIntentSignalHistory p4;
    p4.timestamp_in_seconds = now_in_seconds - (3 * kSecondsPerDay);
    p4.weight = 1;

    PurchaseIntentSignalHistory p5;
    p5.timestamp_in_seconds = now_in_seconds - (2 * kSecondsPerDay);
    p5.weight = 1;

    PurchaseIntentSignalHistory p6;
    p6.timestamp_in_seconds = now_in_seconds - (1 * kSecondsPerDay);
    p6.weight = 2;

    PurchaseIntentSignalHistory p7;
    p7.timestamp_in_seconds = now_in_seconds - (1 * kSecondsPerDay);
    p7.weight = 1;

    PurchaseIntentSignalHistory p8;
    p8.timestamp_in_seconds = now_in_seconds - (1 * kSecondsPerDay);
    p8.weight = 1;

    PurchaseIntentSignalHistory p9;
    p9.timestamp_in_seconds = now_in_seconds - (1 * kSecondsPerDay);
    p9.weight = 1;

    PurchaseIntentSignalHistory p10;
    p10.timestamp_in_seconds = now_in_seconds - (1 * kSecondsPerDay);
    p10.weight = 1;

    PurchaseIntentSignalHistory p11;
    p11.timestamp_in_seconds = now_in_seconds - (1 * kSecondsPerDay);
    p11.weight = 1;

    PurchaseIntentSignalHistory p12;
    p12.timestamp_in_seconds = now_in_seconds - (1 * kSecondsPerDay);
    p12.weight = 8;

    const PurchaseIntentSignalSegmentHistoryMap history = {
      {
        "cat_5", {  // score: 13
          p1,
          p4,
          p8,
          p9,
          p10
        }
      },
      {
        "cat_2", {  // score: 12
          p2,
          p5,
          p8,
          p11
        }
      },
      {
        "cat_1", {  // score: 12
          p3,
          p6,
          p8
        }
      },
      {
        "cat_4", {  // score: 1
          p7
        }
      },
      {
        "cat_3", {  // score: 1
          p8
        }
      },
      {
        "cat_6", {  // score: 8
          p12
        }
      },
      {
        "cat_7", {  // score: 11
          p6,
          p10,
          p12
        }
      }
    };

    return history;
  }

  PurchaseIntentSignalSegmentHistoryMap GenerateShortHistory() {
    const uint64_t now_in_seconds =
        static_cast<uint64_t>(base::Time::Now().ToDoubleT());

    PurchaseIntentSignalHistory p1;
    p1.timestamp_in_seconds = now_in_seconds - (6 * kSecondsPerDay);
    p1.weight = 9;

    PurchaseIntentSignalHistory p2;
    p2.timestamp_in_seconds = now_in_seconds - (5 * kSecondsPerDay);
    p2.weight = 9;

    PurchaseIntentSignalHistory p3;
    p3.timestamp_in_seconds = now_in_seconds - (4 * kSecondsPerDay);
    p3.weight = 9;

    const PurchaseIntentSignalSegmentHistoryMap history = {
      {
        "cat_1", {  // score: 9
          p1
        }
      },
      {
        "cat_2", {  // score: 18
          p2,
          p3
        }
      }
    };

    return history;
  }

  std::unique_ptr<PurchaseIntentClassifier> purchase_intent_classifier_;
};

TEST_F(AdsPurchaseIntentClassifierTest,
    ExtractsPurchaseIntentSignal) {
  for (const auto& search_query : kTestSearchQueries) {
    // Arrange

    // Act
    const PurchaseIntentSignalInfo purchase_intent_signal =
        purchase_intent_classifier_->ExtractIntentSignal(search_query.url);

    // Assert
    const std::vector<std::string> expected_segments = search_query.segments;
    EXPECT_EQ(expected_segments, purchase_intent_signal.segments);

    const uint16_t expected_weight = search_query.weight;
    EXPECT_EQ(expected_weight, purchase_intent_signal.weight);
  }
}

TEST_F(AdsPurchaseIntentClassifierTest,
    GetsWinningCategoriesWithEmptyHistory) {
  // Arrange
  const PurchaseIntentSignalSegmentHistoryMap history = {};

  // Act
  const PurchaseIntentWinningCategoryList winning_categories =
      purchase_intent_classifier_->GetWinningCategories(history, 3);

  // Assert
  const std::vector<std::string> expected_winning_categories = {};
  EXPECT_EQ(expected_winning_categories, winning_categories);
}

TEST_F(AdsPurchaseIntentClassifierTest,
    GetsWinningCategoriesWithShortHistory) {
  // Arrange
  const PurchaseIntentSignalSegmentHistoryMap history = GenerateShortHistory();

  // Act
  const PurchaseIntentWinningCategoryList winning_categories =
      purchase_intent_classifier_->GetWinningCategories(history, 3);

  // Assert
  const std::vector<std::string> expected_winning_categories = {
    "cat_2"
  };

  EXPECT_EQ(expected_winning_categories, winning_categories);
}

TEST_F(AdsPurchaseIntentClassifierTest,
    GetsWinningCategories) {
  // Arrange
  const PurchaseIntentSignalSegmentHistoryMap history = GenerateHistory();

  // Act
  const PurchaseIntentWinningCategoryList winning_categories =
      purchase_intent_classifier_->GetWinningCategories(history, 5);

  // Assert
  const std::vector<std::string> expected_winning_categories = {
    "cat_5",
    "cat_2",
    "cat_1",
    "cat_7"
  };

  EXPECT_EQ(expected_winning_categories, winning_categories);
}

TEST_F(AdsPurchaseIntentClassifierTest,
    GetsWinningCategoriesUpToMaxSegments) {
  // Arrange
  const PurchaseIntentSignalSegmentHistoryMap history = GenerateHistory();

  // Act
  const PurchaseIntentWinningCategoryList winning_categories =
      purchase_intent_classifier_->GetWinningCategories(history, 2);

  // Assert
  const std::vector<std::string> expected_winning_categories = {
    "cat_5",
    "cat_2"
  };

  EXPECT_EQ(expected_winning_categories, winning_categories);
}

}  // namespace ads
