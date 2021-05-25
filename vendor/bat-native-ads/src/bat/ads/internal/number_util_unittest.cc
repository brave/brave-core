/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/number_util.h"

#include <string>

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsNumberUtilTest, DoubleEquals) {
  // Arrange
  const double value = 1.00001;

  // Act
  const bool does_equal = DoubleEquals(value, 1.00002);

  // Assert
  EXPECT_TRUE(does_equal);
}

TEST(BatAdsNumberUtilTest, DoubleNotEquals) {
  // Arrange
  const double value = 1.0001;

  // Act
  const bool does_equal = DoubleEquals(value, 1.0002);

  // Assert
  EXPECT_FALSE(does_equal);
}

TEST(BatAdsNumberUtilTest, DoubleIsGreaterEqual) {
  // Arrange
  const double value = 0.41749999999999687361196265555918216705322265625000;

  // Act
  const bool is_greater_equal = DoubleIsGreaterEqual(
      value, 0.41750000000000014876988529977097641676664352416992);

  // Assert
  EXPECT_TRUE(is_greater_equal);
}

TEST(BatAdsNumberUtilTest, DoubleIsNotGreaterEqual) {
  // Arrange
  const double value = 0.41744999999999687361196265555918216705322265625000;

  // Act
  const bool is_greater_equal = DoubleIsGreaterEqual(
      value, 0.41750000000000014876988529977097641676664352416992);

  // Assert
  EXPECT_FALSE(is_greater_equal);
}

TEST(BatAdsNumberUtilTest, DoubleIsGreater) {
  // Arrange
  const double value = 0.41759999999999687361196265555918216705322265625000;

  // Act
  const bool is_greater = DoubleIsGreater(
      value, 0.41750000000000014876988529977097641676664352416992);

  // Assert
  EXPECT_TRUE(is_greater);
}

TEST(BatAdsNumberUtilTest, DoubleIsNotGreater) {
  // Arrange
  const double value = 0.41749999999999687361196265555918216705322265625000;

  // Act
  const bool is_greater = DoubleIsGreater(
      value, 0.41750000000000014876988529977097641676664352416992);

  // Assert
  EXPECT_FALSE(is_greater);
}

TEST(BatAdsNumberUtilTest, DoubleIsLessEqual) {
  // Arrange
  const double value = 0.41750000000000014876988529977097641676664352416992;

  // Act
  const bool is_less_equal = DoubleIsLessEqual(
      value, 0.41749999999999687361196265555918216705322265625000);

  // Assert
  EXPECT_TRUE(is_less_equal);
}

TEST(BatAdsNumberUtilTest, DoubleIsNotLessEqual) {
  // Arrange
  const double value = 0.41750000000000014876988529977097641676664352416992;

  // Act
  const bool is_less_equal = DoubleIsLessEqual(
      value, 0.41744999999999687361196265555918216705322265625000);

  // Assert
  EXPECT_FALSE(is_less_equal);
}

TEST(BatAdsNumberUtilTest, DoubleIsLess) {
  // Arrange
  const double value = 0.41750000000000014876988529977097641676664352416992;

  // Act
  const bool is_less =
      DoubleIsLess(value, 0.41759999999999687361196265555918216705322265625000);

  // Assert
  EXPECT_TRUE(is_less);
}

TEST(BatAdsNumberUtilTest, DoubleIsNotLess) {
  // Arrange
  const double value = 0.41750000000000014876988529977097641676664352416992;

  // Act
  const bool is_less =
      DoubleIsLess(value, 0.41749999999999687361196265555918216705322265625000);

  // Assert
  EXPECT_FALSE(is_less);
}

}  // namespace ads
