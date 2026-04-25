/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/segments/top_user_model_segments_util.h"

#include <cstddef>

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/intent/intent_user_model_info.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/interest/interest_user_model_info.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/latent_interest/latent_interest_user_model_info.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr size_t kTopSegmentsMaxCount = 2;

UserModelInfo BuildUserModel() {
  return UserModelInfo{
      IntentUserModelInfo{SegmentList{"intent_1_parent-child",
                                      "intent_2_parent-child",
                                      "intent_3_parent-child"}},
      LatentInterestUserModelInfo{SegmentList{
          "latent_interest_1_parent-child", "latent_interest_2_parent-child",
          "latent_interest_3_parent-child"}},
      InterestUserModelInfo{SegmentList{"interest_1_parent-child",
                                        "interest_2_parent-child",
                                        "interest_3_parent-child"}}};
}

}  // namespace

class BraveAdsTopUserModelSegmentsUtilTest : public test::TestBase {};

TEST_F(BraveAdsTopUserModelSegmentsUtilTest, GetTopChildSegments) {
  // Arrange
  const UserModelInfo user_model = BuildUserModel();

  // Act
  const SegmentList top_segments =
      GetTopSegments(user_model, kTopSegmentsMaxCount, /*parent_only=*/false);

  // Assert
  const SegmentList expected_top_segments = {
      "intent_1_parent-child",          "intent_2_parent-child",
      "latent_interest_1_parent-child", "latent_interest_2_parent-child",
      "interest_1_parent-child",        "interest_2_parent-child"};
  EXPECT_EQ(expected_top_segments, top_segments);
}

TEST_F(BraveAdsTopUserModelSegmentsUtilTest, GetTopChildSegmentsIfEmpty) {
  // Act
  const SegmentList top_segments = GetTopSegments(
      /*user_model=*/{}, kTopSegmentsMaxCount, /*parent_only=*/false);

  // Assert
  EXPECT_THAT(top_segments, ::testing::IsEmpty());
}

TEST_F(BraveAdsTopUserModelSegmentsUtilTest, GetTopParentSegmentsIfEmpty) {
  // Act
  const SegmentList top_segments = GetTopSegments(
      /*user_model=*/{}, kTopSegmentsMaxCount, /*parent_only=*/true);

  // Assert
  EXPECT_THAT(top_segments, ::testing::IsEmpty());
}

TEST_F(BraveAdsTopUserModelSegmentsUtilTest, GetTopParentSegments) {
  // Arrange
  const UserModelInfo user_model = BuildUserModel();

  // Act
  const SegmentList top_segments =
      GetTopSegments(user_model, kTopSegmentsMaxCount, /*parent_only=*/true);

  // Assert
  const SegmentList expected_top_segments = {
      "intent_1_parent",          "intent_2_parent",
      "latent_interest_1_parent", "latent_interest_2_parent",
      "interest_1_parent",        "interest_2_parent"};
  EXPECT_EQ(expected_top_segments, top_segments);
}

}  // namespace brave_ads
