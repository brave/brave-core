/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/behavioral/conversions/conversions_resource.h"

#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/resources/behavioral/conversions/conversions_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace resource {

class BatAdsConversionsResourceTest : public UnitTestBase {
 protected:
  BatAdsConversionsResourceTest() = default;

  ~BatAdsConversionsResourceTest() override = default;
};

TEST_F(BatAdsConversionsResourceTest, Load) {
  // Arrange
  Conversions resource;

  // Act
  resource.Load();
  task_environment_.RunUntilIdle();

  // Assert
  EXPECT_TRUE(resource.IsInitialized());
}

TEST_F(BatAdsConversionsResourceTest, Get) {
  // Arrange
  Conversions resource;
  resource.Load();
  task_environment_.RunUntilIdle();

  // Act
  ConversionIdPatternMap conversion_id_patterns = resource.get()->id_patterns;

  // Assert
  EXPECT_EQ(2u, conversion_id_patterns.size());
}

}  // namespace resource
}  // namespace ads
