/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/redeem_unblinded_token/user_data/confirmation_build_channel_dto_user_data.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsConfirmationBuildChannelDtoUserDataTest : public UnitTestBase {
 protected:
  BatAdsConfirmationBuildChannelDtoUserDataTest() = default;

  ~BatAdsConfirmationBuildChannelDtoUserDataTest() override = default;
};

TEST_F(BatAdsConfirmationBuildChannelDtoUserDataTest, GetBuildChannel) {
  // Arrange
  SetBuildChannel(false, "release");

  // Act
  base::DictionaryValue build_channel = dto::user_data::GetBuildChannel();

  // Assert
  base::DictionaryValue expected_build_channel;
  expected_build_channel.SetKey("buildChannel", base::Value("release"));

  EXPECT_EQ(expected_build_channel, build_channel);
}

}  // namespace ads
