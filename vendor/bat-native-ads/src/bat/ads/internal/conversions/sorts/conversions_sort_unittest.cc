/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/conversions/sorts/conversions_sort_factory.h"

#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

ConversionList GetUnsortedConversions() {
  ConversionList list;

  ConversionInfo conversion;
  conversion.type = "postview";
  list.push_back(conversion);
  conversion.type = "postclick";
  list.push_back(conversion);
  conversion.type = "postview";
  list.push_back(conversion);
  conversion.type = "postclick";
  list.push_back(conversion);
  conversion.type = "postview";
  list.push_back(conversion);

  return list;
}

}  // namespace

TEST(BatConversionsSortTest, NoSortOrder) {
  // Arrange

  // Act
  const auto sort = ConversionsSortFactory::Build(ConversionSortType::kNone);

  // Assert
  EXPECT_EQ(nullptr, sort);
}

TEST(BatConversionsSortTest, DescendingSortOrder) {
  // Arrange
  ConversionList list = GetUnsortedConversions();

  const auto sort =
      ConversionsSortFactory::Build(ConversionSortType::kDescendingOrder);

  // Act
  list = sort->Apply(list);

  // Assert
  ConversionList expected_list;
  ConversionInfo conversion;
  conversion.type = "postclick";
  expected_list.push_back(conversion);
  conversion.type = "postclick";
  expected_list.push_back(conversion);
  conversion.type = "postview";
  expected_list.push_back(conversion);
  conversion.type = "postview";
  expected_list.push_back(conversion);
  conversion.type = "postview";
  expected_list.push_back(conversion);

  EXPECT_EQ(expected_list, list);
}

TEST(BatConversionsSortTest, DescendingSortOrderForEmptyList) {
  // Arrange
  ConversionList conversions;

  const auto sort =
      ConversionsSortFactory::Build(ConversionSortType::kDescendingOrder);

  // Act
  conversions = sort->Apply(conversions);

  // Assert
  const ConversionList expected_conversions;

  EXPECT_EQ(expected_conversions, conversions);
}

TEST(BatConversionsSortTest, AscendingSortOrder) {
  // Arrange
  ConversionList list = GetUnsortedConversions();

  const auto sort =
      ConversionsSortFactory::Build(ConversionSortType::kAscendingOrder);

  // Act
  list = sort->Apply(list);

  // Assert
  ConversionList expected_list;
  ConversionInfo conversion;
  conversion.type = "postview";
  expected_list.push_back(conversion);
  conversion.type = "postview";
  expected_list.push_back(conversion);
  conversion.type = "postview";
  expected_list.push_back(conversion);
  conversion.type = "postclick";
  expected_list.push_back(conversion);
  conversion.type = "postclick";
  expected_list.push_back(conversion);

  EXPECT_EQ(expected_list, list);
}

TEST(BatConversionsSortTest, AscendingSortOrderForEmptyList) {
  // Arrange
  ConversionList conversions;

  const auto sort =
      ConversionsSortFactory::Build(ConversionSortType::kAscendingOrder);

  // Act
  conversions = sort->Apply(conversions);

  // Assert
  const ConversionList expected_conversions;

  EXPECT_EQ(expected_conversions, conversions);
}

}  // namespace ads
