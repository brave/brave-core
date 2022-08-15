/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"

#include "bat/ads/internal/base/unittest/unittest_base.h"

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
  AntiTargeting resource;

  // Act
  resource.Load();
  task_environment_.RunUntilIdle();

  // Assert
  EXPECT_TRUE(resource.IsInitialized());
}

}  // namespace resource
}  // namespace ads
