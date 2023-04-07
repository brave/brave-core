/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/url/request_builder/host/url_host_util.h"
#include "brave/components/brave_ads/core/internal/flags/environment/environment_types.h"
#include "brave/components/brave_ads/core/internal/flags/flag_manager.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

class BatAdsStaticUrlHostTest : public UnitTestBase {};

TEST_F(BatAdsStaticUrlHostTest, GetProductionUrlHost) {
  // Arrange
  FlagManager::GetInstance()->SetEnvironmentTypeForTesting(
      EnvironmentType::kProduction);

  // Act
  const std::string url_host = GetStaticUrlHost();

  // Assert
  const std::string expected_url_host = "https://static.ads.brave.com";
  EXPECT_EQ(expected_url_host, url_host);
}

TEST_F(BatAdsStaticUrlHostTest, GetStagingUrlHost) {
  // Arrange
  FlagManager::GetInstance()->SetEnvironmentTypeForTesting(
      EnvironmentType::kStaging);

  // Act
  const std::string url_host = GetStaticUrlHost();

  // Assert
  const std::string expected_url_host = "https://static.ads.bravesoftware.com";
  EXPECT_EQ(expected_url_host, url_host);
}

}  // namespace brave_ads
