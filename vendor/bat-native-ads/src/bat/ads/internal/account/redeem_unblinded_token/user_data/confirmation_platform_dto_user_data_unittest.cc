/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/redeem_unblinded_token/user_data/confirmation_platform_dto_user_data.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsConfirmationPlatformDtoUserDataTest : public UnitTestBase {
 protected:
  BatAdsConfirmationPlatformDtoUserDataTest() = default;

  ~BatAdsConfirmationPlatformDtoUserDataTest() override = default;
};

TEST_F(BatAdsConfirmationPlatformDtoUserDataTest, GetPlatform) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kMacOS);

  // Act
  base::DictionaryValue platform = dto::user_data::GetPlatform();

  // Assert
  base::DictionaryValue expected_platform;
  expected_platform.SetKey("platform", base::Value("macos"));

  EXPECT_EQ(expected_platform, platform);
}

}  // namespace ads
