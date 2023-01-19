/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/flags/environment/environment_types.h"
#include "bat/ads/internal/flags/flag_manager.h"
#include "bat/ads/internal/server/url/hosts/server_host_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsStaticServerHostTest : public UnitTestBase {};

TEST_F(BatAdsStaticServerHostTest, GetProductionHost) {
  // Arrange
  FlagManager::GetInstance()->SetEnvironmentTypeForTesting(
      EnvironmentType::kProduction);

  // Act
  const std::string host = server::GetStaticHost();

  // Assert
  const std::string expected_host = "https://static.ads.brave.com";
  EXPECT_EQ(expected_host, host);
}

TEST_F(BatAdsStaticServerHostTest, GetStagingHost) {
  // Arrange
  FlagManager::GetInstance()->SetEnvironmentTypeForTesting(
      EnvironmentType::kStaging);

  // Act
  const std::string host = server::GetStaticHost();

  // Assert
  const std::string expected_host = "https://static.ads.bravesoftware.com";
  EXPECT_EQ(expected_host, host);
}

}  // namespace ads
