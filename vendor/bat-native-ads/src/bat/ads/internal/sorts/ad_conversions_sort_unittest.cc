/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/sorts/ad_conversions_sort_factory.h"
#include "bat/ads/internal/client_mock.h"
#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/ads_impl.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using std::placeholders::_1;

namespace ads {

class BatAdConversionsSortTest : public ::testing::Test {
 protected:
  BatAdConversionsSortTest()
      : mock_ads_client_(std::make_unique<MockAdsClient>()),
        ads_(std::make_unique<AdsImpl>(mock_ads_client_.get())) {
    // You can do set-up work for each test here
  }

  ~BatAdConversionsSortTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)

    auto callback = std::bind(
        &BatAdConversionsSortTest::OnAdsImplInitialize, this, _1);
    ads_->Initialize(callback);

    client_mock_ =
        std::make_unique<ClientMock>(ads_.get(), mock_ads_client_.get());
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  void OnAdsImplInitialize(const Result result) {
    EXPECT_EQ(Result::SUCCESS, result);
  }

  AdConversionList GetUnsortedAdConversions() {
    AdConversionList list;

    AdConversionInfo info;
    info.type = "postview";
    list.push_back(info);
    info.type = "postclick";
    list.push_back(info);
    info.type = "postview";
    list.push_back(info);
    info.type = "postclick";
    list.push_back(info);
    info.type = "postview";
    list.push_back(info);

    return list;
  }

  std::unique_ptr<MockAdsClient> mock_ads_client_;
  std::unique_ptr<AdsImpl> ads_;

  std::unique_ptr<ClientMock> client_mock_;
};

TEST_F(BatAdConversionsSortTest,
    NoSortOrder) {
  // Arrange

  // Act
  const auto sort =
      AdConversionsSortFactory::Build(AdConversionInfo::SortType::kNone);

  // Assert
  ASSERT_EQ(sort, nullptr);
}

TEST_F(BatAdConversionsSortTest,
    DescendingSortOrder) {
  // Arrange
  AdConversionList list = GetUnsortedAdConversions();

  const auto sort = AdConversionsSortFactory::Build(
      AdConversionInfo::SortType::kDescendingOrder);

  // Act
  list = sort->Apply(list);

  // Assert
  AdConversionList expected_list;
  AdConversionInfo info;
  info.type = "postclick";
  expected_list.push_back(info);
  info.type = "postclick";
  expected_list.push_back(info);
  info.type = "postview";
  expected_list.push_back(info);
  info.type = "postview";
  expected_list.push_back(info);
  info.type = "postview";
  expected_list.push_back(info);

  ASSERT_EQ(expected_list, list);
}

TEST_F(BatAdConversionsSortTest,
    DescendingSortOrderForEmptyList) {
  // Arrange
  const auto sort = AdConversionsSortFactory::Build(
      AdConversionInfo::SortType::kDescendingOrder);

  AdConversionList list;

  // Act
  list = sort->Apply(list);

  // Assert
  AdConversionList expected_list;

  ASSERT_EQ(expected_list, list);
}

TEST_F(BatAdConversionsSortTest,
    AscendingSortOrder) {
  // Arrange
  AdConversionList list = GetUnsortedAdConversions();

  const auto sort = AdConversionsSortFactory::Build(
      AdConversionInfo::SortType::kAscendingOrder);

  // Act
  list = sort->Apply(list);

  // Assert
  AdConversionList expected_list;
  AdConversionInfo info;
  info.type = "postview";
  expected_list.push_back(info);
  info.type = "postview";
  expected_list.push_back(info);
  info.type = "postview";
  expected_list.push_back(info);
  info.type = "postclick";
  expected_list.push_back(info);
  info.type = "postclick";
  expected_list.push_back(info);

  ASSERT_EQ(expected_list, list);
}

TEST_F(BatAdConversionsSortTest,
    AscendingSortOrderForEmptyList) {
  // Arrange
  const auto sort = AdConversionsSortFactory::Build(
      AdConversionInfo::SortType::kAscendingOrder);

  AdConversionList list;

  // Act
  list = sort->Apply(list);

  // Assert
  AdConversionList expected_list;

  ASSERT_EQ(expected_list, list);
}

}  // namespace ads
