/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/conversions/conversions_resource.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

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

  // Assert
  const bool is_initialized = resource.IsInitialized();
  EXPECT_TRUE(is_initialized);
}

TEST_F(BatAdsConversionsResourceTest, Get) {
  // Arrange
  Conversions resource;
  resource.Load();

  // Act
  ConversionIdPatternMap conversion_id_patterns = resource.get();

  // Assert
  EXPECT_EQ(2u, conversion_id_patterns.size());
}

}  // namespace resource
}  // namespace ads
