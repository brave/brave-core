/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/sorts/ads_history_sort_factory.h"

#include <deque>

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

std::deque<AdHistory> GetUnsortedAdsHistory() {
  std::deque<AdHistory> ads_history;

  AdHistory ad_history;
  ad_history.timestamp_in_seconds = 22222222222;
  ads_history.push_back(ad_history);
  ad_history.timestamp_in_seconds = 33333333333;
  ads_history.push_back(ad_history);
  ad_history.timestamp_in_seconds = 11111111111;
  ads_history.push_back(ad_history);
  ad_history.timestamp_in_seconds = 55555555555;
  ads_history.push_back(ad_history);
  ad_history.timestamp_in_seconds = 44444444444;
  ads_history.push_back(ad_history);

  return ads_history;
}

}  // namespace

TEST(BatAdsAdsHistorySortTest,
    NoSortOrder) {
  // Arrange

  // Act
  const auto sort = AdsHistorySortFactory::Build(AdsHistory::SortType::kNone);

  // Assert
  EXPECT_EQ(nullptr, sort);
}

TEST(BatAdsAdsHistorySortTest,
    DescendingSortOrder) {
  // Arrange
  const auto sort = AdsHistorySortFactory::Build(
      AdsHistory::SortType::kDescendingOrder);

  std::deque<AdHistory> history = GetUnsortedAdsHistory();

  // Act
  history = sort->Apply(history);

  // Assert
  std::deque<AdHistory> expected_history;
  AdHistory ad_history;
  ad_history.timestamp_in_seconds = 55555555555;
  expected_history.push_back(ad_history);
  ad_history.timestamp_in_seconds = 44444444444;
  expected_history.push_back(ad_history);
  ad_history.timestamp_in_seconds = 33333333333;
  expected_history.push_back(ad_history);
  ad_history.timestamp_in_seconds = 22222222222;
  expected_history.push_back(ad_history);
  ad_history.timestamp_in_seconds = 11111111111;
  expected_history.push_back(ad_history);

  EXPECT_EQ(expected_history, history);
}

TEST(BatAdsAdsHistorySortTest,
    DescendingSortOrderForEmptyHistory) {
  // Arrange
  const auto sort = AdsHistorySortFactory::Build(
      AdsHistory::SortType::kDescendingOrder);

  std::deque<AdHistory> history = {};

  // Act
  history = sort->Apply(history);

  // Assert
  const std::deque<AdHistory> expected_history = {};

  EXPECT_EQ(expected_history, history);
}

TEST(BatAdsAdsHistorySortTest,
    AscendingSortOrder) {
  // Arrange
  const auto sort = AdsHistorySortFactory::Build(
      AdsHistory::SortType::kAscendingOrder);

  std::deque<AdHistory> expected_history;
  AdHistory ad_history;
  ad_history.timestamp_in_seconds = 11111111111;
  expected_history.push_back(ad_history);
  ad_history.timestamp_in_seconds = 22222222222;
  expected_history.push_back(ad_history);
  ad_history.timestamp_in_seconds = 33333333333;
  expected_history.push_back(ad_history);
  ad_history.timestamp_in_seconds = 44444444444;
  expected_history.push_back(ad_history);
  ad_history.timestamp_in_seconds = 55555555555;
  expected_history.push_back(ad_history);

  std::deque<AdHistory> history = GetUnsortedAdsHistory();

  // Act
  history = sort->Apply(history);

  // Assert
  EXPECT_EQ(expected_history, history);
}

TEST(BatAdsAdsHistorySortTest,
    AscendingSortOrderForEmptyHistory) {
  // Arrange
  const auto sort = AdsHistorySortFactory::Build(
      AdsHistory::SortType::kAscendingOrder);

  std::deque<AdHistory> expected_history;
  std::deque<AdHistory> history;

  // Act
  history = sort->Apply(history);

  // Assert
  EXPECT_EQ(expected_history, history);
}

}  // namespace ads
