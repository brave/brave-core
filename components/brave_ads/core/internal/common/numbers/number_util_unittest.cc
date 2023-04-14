/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/numbers/number_util.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

TEST(BatAdsNumberUtilTest, DoubleEquals) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(DoubleEquals(1.00001, 1.00002));
}

TEST(BatAdsNumberUtilTest, DoubleNotEquals) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(DoubleEquals(1.0001, 1.0002));
}

TEST(BatAdsNumberUtilTest, DoubleIsGreaterEqual) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(DoubleIsGreaterEqual(
      0.41749999999999687361196265555918216705322265625000,
      0.41750000000000014876988529977097641676664352416992));
}

TEST(BatAdsNumberUtilTest, DoubleIsNotGreaterEqual) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(DoubleIsGreaterEqual(
      0.41744999999999687361196265555918216705322265625000,
      0.41750000000000014876988529977097641676664352416992));
}

TEST(BatAdsNumberUtilTest, DoubleIsGreater) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(
      DoubleIsGreater(0.41759999999999687361196265555918216705322265625000,
                      0.41750000000000014876988529977097641676664352416992));
}

TEST(BatAdsNumberUtilTest, DoubleIsNotGreater) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(
      DoubleIsGreater(0.41749999999999687361196265555918216705322265625000,
                      0.41750000000000014876988529977097641676664352416992));
}

TEST(BatAdsNumberUtilTest, DoubleIsLessEqual) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(
      DoubleIsLessEqual(0.41750000000000014876988529977097641676664352416992,
                        0.41749999999999687361196265555918216705322265625000));
}

TEST(BatAdsNumberUtilTest, DoubleIsNotLessEqual) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(
      DoubleIsLessEqual(0.41750000000000014876988529977097641676664352416992,
                        0.41744999999999687361196265555918216705322265625000));
}

TEST(BatAdsNumberUtilTest, DoubleIsLess) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(
      DoubleIsLess(0.41750000000000014876988529977097641676664352416992,
                   0.41759999999999687361196265555918216705322265625000));
}

TEST(BatAdsNumberUtilTest, DoubleIsNotLess) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(
      DoubleIsLess(0.41750000000000014876988529977097641676664352416992,
                   0.41749999999999687361196265555918216705322265625000));
}

}  // namespace brave_ads
