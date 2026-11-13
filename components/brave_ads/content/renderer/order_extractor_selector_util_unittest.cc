/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/renderer/order_extractor_selector_util.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/re2/src/re2/re2.h"

// pnpm test brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsOrderExtractorSelectorUtilTest,
     ExtractConcatenatedGroupsReturnsNulloptForZeroGroups) {
  // Arrange
  const re2::RE2 regex(R"RE2(xvpn:sub:[0-9a-f-]+)RE2", re2::RE2::Quiet);
  ASSERT_TRUE(regex.ok());

  // Act & Assert
  EXPECT_FALSE(MaybeExtractConcatenatedGroups("xvpn:sub:deadbeef", regex));
}

TEST(BraveAdsOrderExtractorSelectorUtilTest,
     ExtractConcatenatedGroupsReturnsSoleGroup) {
  // Arrange
  const re2::RE2 regex(R"RE2(xvpn:sub:([0-9a-f-]+))RE2", re2::RE2::Quiet);
  ASSERT_TRUE(regex.ok());

  // Act & Assert
  EXPECT_EQ("deadbeef",
            MaybeExtractConcatenatedGroups("xvpn:sub:deadbeef", regex));
}

TEST(BraveAdsOrderExtractorSelectorUtilTest,
     ExtractConcatenatedGroupsConcatenatesMultipleGroupsInOrder) {
  // Arrange
  const re2::RE2 regex(R"RE2(^([A-Z]{3})([0-9]{5})$)RE2", re2::RE2::Quiet);
  ASSERT_TRUE(regex.ok());

  // Act & Assert
  EXPECT_EQ("ABC12345", MaybeExtractConcatenatedGroups("ABC12345", regex));
}

TEST(BraveAdsOrderExtractorSelectorUtilTest,
     ExtractConcatenatedGroupsSkipsUnmatchedOptionalGroup) {
  // Arrange
  const re2::RE2 regex(R"RE2(^([A-Z]{3})(-X)?([0-9]{5})$)RE2", re2::RE2::Quiet);
  ASSERT_TRUE(regex.ok());

  // Act & Assert
  EXPECT_EQ("ABC12345", MaybeExtractConcatenatedGroups("ABC12345", regex));
}

TEST(BraveAdsOrderExtractorSelectorUtilTest,
     ExtractConcatenatedGroupsMatchesPartOfText) {
  // Arrange
  const re2::RE2 regex(R"RE2(order-id=([0-9]+))RE2", re2::RE2::Quiet);
  ASSERT_TRUE(regex.ok());

  // Act & Assert
  EXPECT_EQ("42",
            MaybeExtractConcatenatedGroups("prefix order-id=42 suffix", regex));
}

TEST(BraveAdsOrderExtractorSelectorUtilTest,
     ExtractConcatenatedGroupsReturnsNulloptIfNoMatch) {
  // Arrange
  const re2::RE2 regex(R"RE2(xvpn:sub:([0-9a-f-]+))RE2", re2::RE2::Quiet);
  ASSERT_TRUE(regex.ok());

  // Act & Assert
  EXPECT_FALSE(MaybeExtractConcatenatedGroups("no match here", regex));
}

TEST(BraveAdsOrderExtractorSelectorUtilTest,
     ExtractConcatenatedGroupsReturnsNulloptIfEveryGroupIsEmpty) {
  // Arrange
  const re2::RE2 regex(R"RE2(^(a*)(b*)$)RE2", re2::RE2::Quiet);
  ASSERT_TRUE(regex.ok());

  // Act & Assert
  EXPECT_FALSE(MaybeExtractConcatenatedGroups("", regex));
}

}  // namespace brave_ads
