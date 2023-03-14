/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::resource {

class BatAdsAntiTargetingResourceTest : public UnitTestBase {};

TEST_F(BatAdsAntiTargetingResourceTest, Load) {
  // Arrange
  AntiTargeting resource;

  // Act
  resource.Load();
  task_environment_.RunUntilIdle();

  // Assert
  EXPECT_TRUE(resource.IsInitialized());
}

}  // namespace ads::resource
