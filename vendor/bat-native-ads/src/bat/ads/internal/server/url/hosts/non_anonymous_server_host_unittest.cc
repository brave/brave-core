/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/flags/environment/environment_types.h"
#include "bat/ads/internal/flags/flag_manager_util.h"
#include "bat/ads/internal/server/url/hosts/server_host_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsNonAnonymousServerHostTest : public UnitTestBase {
 protected:
  BatAdsNonAnonymousServerHostTest() = default;

  ~BatAdsNonAnonymousServerHostTest() override = default;
};

TEST_F(BatAdsNonAnonymousServerHostTest, GetProductionHost) {
  // Arrange
  SetEnvironmentTypeForTesting(EnvironmentType::kProduction);

  // Act
  const std::string host = server::GetNonAnonymousHost();

  // Assert
  const std::string expected_host = "https://mywallet.ads.brave.com";
  EXPECT_EQ(expected_host, host);
}

TEST_F(BatAdsNonAnonymousServerHostTest, GetStagingHost) {
  // Arrange
  SetEnvironmentTypeForTesting(EnvironmentType::kStaging);

  // Act
  const std::string host = server::GetNonAnonymousHost();

  // Assert
  const std::string expected_host = "https://mywallet.ads.bravesoftware.com";
  EXPECT_EQ(expected_host, host);
}

}  // namespace ads
