/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/number_util.h"

#include <string>

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsNumberUtilTest,
    DoubleEquals) {
  // Arrange
  const double value = 1.0;

  // Act
  const bool does_equal = DoubleEquals(value, 1.0);

  // Assert
  EXPECT_TRUE(does_equal);
}

TEST(BatAdsNumberUtilTest,
    DoubleNotEquals) {
  // Arrange
  const double value = 1.0;

  // Act
  const bool does_equal = DoubleEquals(value, 0.9);

  // Assert
  EXPECT_FALSE(does_equal);
}

TEST(BatAdsNumberUtilTest,
    DoubleEqualsBelowEpsilon) {
  // Arrange
  const double value = 0.9999;

  // Act
  const bool does_equal = DoubleEquals(value, 1.0);

  // Assert
  EXPECT_TRUE(does_equal);
}

}  // namespace ads
