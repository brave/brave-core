/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads_impl.h"

#include <memory>

#include "base/test/task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/unittest_utils.h"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SetArgPointee;

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsIsMobileTest : public ::testing::Test {
 protected:
  BatAdsIsMobileTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        ads_client_mock_(std::make_unique<NiceMock<AdsClientMock>>()),
        ads_(std::make_unique<AdsImpl>(ads_client_mock_.get())) {
    // You can do set-up work for each test here
  }

  ~BatAdsIsMobileTest() override {
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

TEST_F(BatAdsIsMobileTest,
    IosIsMobile) {
  // Arrange
  ClientInfo client_info;
  client_info.platform = IOS;

  // Act
  EXPECT_CALL(*ads_client_mock_, GetClientInfo(_))
      .WillOnce(SetArgPointee<0>(client_info));

  // Assert
  EXPECT_TRUE(ads_->IsMobile());
}

TEST_F(BatAdsIsMobileTest,
    AndroidIsMobile) {
  // Arrange
  ClientInfo client_info;
  client_info.platform = ANDROID_OS;

  // Act
  EXPECT_CALL(*ads_client_mock_, GetClientInfo(_))
      .WillOnce(SetArgPointee<0>(client_info));

  // Assert
  EXPECT_TRUE(ads_->IsMobile());
}

TEST_F(BatAdsIsMobileTest,
    WindowsIsDesktop) {
  // Arrange
  ClientInfo client_info;
  client_info.platform = WINDOWS;

  // Act
  EXPECT_CALL(*ads_client_mock_, GetClientInfo(_))
      .WillOnce(SetArgPointee<0>(client_info));

  // Assert
  EXPECT_FALSE(ads_->IsMobile());
}

TEST_F(BatAdsIsMobileTest,
    MacOsIsDesktop) {
  // Arrange
  ClientInfo client_info;
  client_info.platform = MACOS;

  // Act
  EXPECT_CALL(*ads_client_mock_, GetClientInfo(_))
      .WillOnce(SetArgPointee<0>(client_info));

  // Assert
  EXPECT_FALSE(ads_->IsMobile());
}

TEST_F(BatAdsIsMobileTest,
    LinuxIsDesktop) {
  // Arrange
  ClientInfo client_info;
  client_info.platform = LINUX;

  // Act
  EXPECT_CALL(*ads_client_mock_, GetClientInfo(_))
      .WillOnce(SetArgPointee<0>(client_info));

  // Assert
  EXPECT_FALSE(ads_->IsMobile());
}

}  // namespace ads
