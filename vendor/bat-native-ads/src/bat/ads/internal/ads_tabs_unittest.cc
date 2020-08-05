/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads_impl.h"

#include <memory>

#include "base/test/task_environment.h"
#include "brave/components/l10n/browser/locale_helper_mock.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/unittest_util.h"

using ::testing::_;
using ::testing::NiceMock;

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsTabsTest : public ::testing::Test {
 protected:
  BatAdsTabsTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        ads_client_mock_(std::make_unique<NiceMock<AdsClientMock>>()),
        ads_(std::make_unique<AdsImpl>(ads_client_mock_.get())) {
    // You can do set-up work for each test here
  }

  ~BatAdsTabsTest() override {
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

  base::test::TaskEnvironment task_environment_;

  std::unique_ptr<AdsClientMock> ads_client_mock_;
  std::unique_ptr<AdsImpl> ads_;
};

TEST_F(BatAdsTabsTest,
    MediaIsPlaying) {
  // Arrange
  ads_->OnTabUpdated(1, "https://brave.com", true, false);
  ads_->OnMediaPlaying(1);

  // Act
  const bool is_media_playing = ads_->IsMediaPlaying();

  // Assert
  EXPECT_TRUE(is_media_playing);
}

TEST_F(BatAdsTabsTest,
    MediaIsNotPlaying) {
  // Arrange
  ads_->OnTabUpdated(1, "https://brave.com", true, false);

  ads_->OnMediaPlaying(1);
  ads_->OnMediaPlaying(2);

  ads_->OnMediaStopped(1);
  ads_->OnMediaStopped(2);

  // Act
  const bool is_playing = ads_->IsMediaPlaying();

  // Assert
  EXPECT_FALSE(is_playing);
}

TEST_F(BatAdsTabsTest,
    IncognitoTabUpdated) {
  // Arrange
  EXPECT_CALL(*ads_client_mock_, Log(_, _, _, _))
      .Times(0);

  // Act
  ads_->OnTabUpdated(1, "https://brave.com", true, true);

  // Assert
}

TEST_F(BatAdsTabsTest,
    InactiveIncognitoTabUpdated) {
  // Arrange
  EXPECT_CALL(*ads_client_mock_, Log(_, _, _, _))
      .Times(0);

  // Act
  ads_->OnTabUpdated(1, "https://brave.com", false, true);

  // Assert
}

TEST_F(BatAdsTabsTest,
    TabUpdated) {
  // Arrange
  EXPECT_CALL(*ads_client_mock_, Log(_, _, _, _))
      .Times(2);

  // Act
  ads_->OnTabUpdated(1, "https://brave.com", true, false);

  // Assert
}

TEST_F(BatAdsTabsTest,
    InactiveTabUpdated) {
  // Arrange
  EXPECT_CALL(*ads_client_mock_, Log(_, _, _, _))
      .Times(2);

  // Act
  ads_->OnTabUpdated(1, "https://brave.com", false, false);

  // Assert
}

TEST_F(BatAdsTabsTest,
    TabClosedWhileMediaIsPlaying) {
  // Arrange
  ads_->OnMediaPlaying(1);

  // Act
  ads_->OnTabClosed(1);

  // Assert
  EXPECT_FALSE(ads_->IsMediaPlaying());
}

}  // namespace ads
