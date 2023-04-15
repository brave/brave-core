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

class BatAdsGeoUrlHostTest : public UnitTestBase {};

TEST_F(BatAdsGeoUrlHostTest, GetProductionUrlHost) {
  // Arrange
  FlagManager::GetInstance()->SetEnvironmentTypeForTesting(
      EnvironmentType::kProduction);

  // Act

  // Assert
  EXPECT_EQ("https://geo.ads.brave.com", GetGeoUrlHost());
}

TEST_F(BatAdsGeoUrlHostTest, GetStagingUrlHost) {
  // Arrange
  FlagManager::GetInstance()->SetEnvironmentTypeForTesting(
      EnvironmentType::kStaging);

  // Act

  // Assert
  EXPECT_EQ("https://geo.ads.bravesoftware.com", GetGeoUrlHost());
}

}  // namespace brave_ads
