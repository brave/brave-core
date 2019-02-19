/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mock_ads_client.h"
#include "src/ads_impl.h"

using ::testing::_;
using ::testing::Invoke;

namespace ads {

class IsMobileTest : public ::testing::Test {
 protected:
  MockAdsClient *mock_ads_client;
  AdsImpl* ads;

  IsMobileTest() :
      mock_ads_client(new MockAdsClient()),
      ads(new AdsImpl(mock_ads_client)) {
    // You can do set-up work for each test here
  }

  ~IsMobileTest() override {
    // You can do clean-up work that doesn't throw exceptions here

    delete ads;
    delete mock_ads_client;
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

TEST_F(IsMobileTest, IsIOSMobile) {
  ClientInfo dummy_client_info;
  dummy_client_info.platform = IOS;

  EXPECT_CALL(*mock_ads_client, GetClientInfo(_))
      .WillOnce(testing::SetArgPointee<0>(dummy_client_info));

  EXPECT_TRUE(ads->IsMobile());
}

TEST_F(IsMobileTest, IsAndroidMobile) {
  ClientInfo dummy_client_info;
  dummy_client_info.platform = ANDROID_OS;

  EXPECT_CALL(*mock_ads_client, GetClientInfo(_))
      .WillOnce(testing::SetArgPointee<0>(dummy_client_info));

  EXPECT_TRUE(ads->IsMobile());
}

TEST_F(IsMobileTest, IsWin7Desktop) {
  ClientInfo dummy_client_info;
  dummy_client_info.platform = WIN7;

  EXPECT_CALL(*mock_ads_client, GetClientInfo(_))
      .WillOnce(testing::SetArgPointee<0>(dummy_client_info));

  EXPECT_FALSE(ads->IsMobile());
}

TEST_F(IsMobileTest, IsWin8Desktop) {
  ClientInfo dummy_client_info;
  dummy_client_info.platform = WIN8;

  EXPECT_CALL(*mock_ads_client, GetClientInfo(_))
      .WillOnce(testing::SetArgPointee<0>(dummy_client_info));

  EXPECT_FALSE(ads->IsMobile());
}

TEST_F(IsMobileTest, IsWin10Desktop) {
  ClientInfo dummy_client_info;
  dummy_client_info.platform = WIN10;

  EXPECT_CALL(*mock_ads_client, GetClientInfo(_))
      .WillOnce(testing::SetArgPointee<0>(dummy_client_info));

  EXPECT_FALSE(ads->IsMobile());
}

TEST_F(IsMobileTest, IsMacOSDesktop) {
  ClientInfo dummy_client_info;
  dummy_client_info.platform = MACOS;

  EXPECT_CALL(*mock_ads_client, GetClientInfo(_))
      .WillOnce(testing::SetArgPointee<0>(dummy_client_info));

  EXPECT_FALSE(ads->IsMobile());
}

TEST_F(IsMobileTest, IsLinuxDesktop) {
  ClientInfo dummy_client_info;
  dummy_client_info.platform = LINUX;

  EXPECT_CALL(*mock_ads_client, GetClientInfo(_))
      .WillOnce(testing::SetArgPointee<0>(dummy_client_info));

  EXPECT_FALSE(ads->IsMobile());
}

}  // namespace ads
