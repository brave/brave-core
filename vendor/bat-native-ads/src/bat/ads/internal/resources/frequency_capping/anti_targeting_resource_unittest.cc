/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/frequency_capping/anti_targeting_resource.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace resource {

class BatAdsAntiTargetingResourceTest : public UnitTestBase {
 protected:
  BatAdsAntiTargetingResourceTest() = default;

  ~BatAdsAntiTargetingResourceTest() override = default;
};

TEST_F(BatAdsAntiTargetingResourceTest, Load) {
  // Arrange
  resource::AntiTargeting resource;

  // Act
  resource.Load();

  // Assert
  const bool is_initialized = resource.IsInitialized();
  EXPECT_TRUE(is_initialized);
}

}  // namespace resource
}  // namespace ads
