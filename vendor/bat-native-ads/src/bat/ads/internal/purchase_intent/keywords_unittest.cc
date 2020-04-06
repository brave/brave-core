/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdint.h>
#include <memory>
#include <tuple>

#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/ads_impl.h"

#include "testing/gtest/include/gtest/gtest.h"

#include "bat/ads/internal/purchase_intent/keywords.h"

using ::testing::_;

// npm run test -- brave_unit_tests --filter=AdsPurchaseIntentKeywords*

namespace {

const std::vector<std::string> kAudiSegments = {
  "automotive purchase intent by make-audi"
};

const std::vector<std::string> kBmwSegments = {
  "automotive purchase intent by make-bmw"
};

const std::vector<std::string> kAudiA4Segments = {
  "automotive purchase intent by make-audi",
  "automotive purchase intent by category-entry luxury car"
};

const std::vector<std::string> kAudiA5Segments = {
  "automotive purchase intent by make-audi",
  "automotive purchase intent by category-entry luxury car"
};

const std::vector<std::string> kAudiA6Segments = {
  "automotive purchase intent by make-audi",
  "automotive purchase intent by category-mid luxury car"
};

const std::vector<std::string> kNoSegments;

struct TestTriplet {
  std::string keywords;
  std::vector<std::string> segments;
  uint16_t weight;
};

const std::vector<TestTriplet> kTestSearchqueries = {
  {"BMW", kBmwSegments, 1},
  {"latest audi a6 review", kAudiA6Segments, 2},
  {"  \tlatest audi\na6 !?# @& review  \t  ", kAudiA6Segments, 2},
  {"latest audi a4 dealer reviews", kAudiA4Segments, 3},
  {"latest audi a6 ", kAudiA6Segments, 1},
  {"this is a test", kNoSegments, 1},
  {"audi a5 dealer opening times sell", kAudiA5Segments, 3},
  {"audi", kAudiSegments, 1},
};

}  // namespace

namespace ads {

class AdsPurchaseIntentKeywordsTest : public ::testing::Test {
 protected:
  std::unique_ptr<MockAdsClient> mock_ads_client_;
  std::unique_ptr<AdsImpl> ads_;

  AdsPurchaseIntentKeywordsTest() :
      mock_ads_client_(std::make_unique<MockAdsClient>()),
      ads_(std::make_unique<AdsImpl>(mock_ads_client_.get())) {
    // You can do set-up work for each test here
  }

  ~AdsPurchaseIntentKeywordsTest() override {
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
};

TEST_F(AdsPurchaseIntentKeywordsTest, MatchesSegmentKeywords) {
  for (const auto& search_query : kTestSearchqueries) {
    // Arrange
    auto query = search_query.keywords;
    auto true_segments = search_query.segments;

    // Act
    auto matched_segments = Keywords::GetSegments(query);

    // Assert
    EXPECT_EQ(true_segments, matched_segments);
  }
}

TEST_F(AdsPurchaseIntentKeywordsTest, MatchesFunnelKeywords) {
  for (const auto& search_query : kTestSearchqueries) {
    // Arrange
    auto query = search_query.keywords;
    auto actual_weight = search_query.weight;

    // Act
    uint16_t keyword_weight = Keywords::GetFunnelWeight(query);

    // Assert
    EXPECT_EQ(actual_weight, keyword_weight);
  }
}

}  // namespace ads
