/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/segments/top_user_model_segments.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/intent/intent_user_model_info.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/interest/interest_user_model_info.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/latent_interest/latent_interest_user_model_info.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

UserModelInfo BuildUserModel() {
  return UserModelInfo{
      IntentUserModelInfo{
          SegmentList{"intent_1_parent-child", "intent_2_parent-child"}},
      LatentInterestUserModelInfo{SegmentList{
          "latent_interest_1_parent-child", "latent_interest_2_parent-child"}},
      InterestUserModelInfo{
          SegmentList{"interest_1_parent-child", "interest_2_parent-child"}}};
}

}  // namespace

class BraveAdsTopUserModelSegmentsTest : public UnitTestBase {};

TEST_F(BraveAdsTopUserModelSegmentsTest, GetTopChildSegments) {
  // Act & Assert
  const SegmentList expected_top_segments = {
      "intent_1_parent-child",          "intent_2_parent-child",
      "latent_interest_1_parent-child", "latent_interest_2_parent-child",
      "interest_1_parent-child",        "interest_2_parent-child"};
  EXPECT_EQ(expected_top_segments, GetTopChildSegments(BuildUserModel()));
}

TEST_F(BraveAdsTopUserModelSegmentsTest, GetTopParentSegments) {
  // Act & Assert
  const SegmentList expected_top_segments = {
      "intent_1_parent",          "intent_2_parent",
      "latent_interest_1_parent", "latent_interest_2_parent",
      "interest_1_parent",        "interest_2_parent"};
  EXPECT_EQ(expected_top_segments, GetTopParentSegments(BuildUserModel()));
}

TEST_F(BraveAdsTopUserModelSegmentsTest, GetTopChildIntentSegments) {
  // Act & Assert
  const SegmentList expected_top_segments = {"intent_1_parent-child",
                                             "intent_2_parent-child"};
  EXPECT_EQ(expected_top_segments, GetTopChildIntentSegments(BuildUserModel()));
}

TEST_F(BraveAdsTopUserModelSegmentsTest, GetTopParentIntentSegments) {
  // Act & Assert
  const SegmentList expected_top_segments = {"intent_1_parent",
                                             "intent_2_parent"};
  EXPECT_EQ(expected_top_segments,
            GetTopParentIntentSegments(BuildUserModel()));
}

TEST_F(BraveAdsTopUserModelSegmentsTest, GetTopChildLatentInterestSegments) {
  // Act & Assert
  const SegmentList expected_top_segments = {"latent_interest_1_parent-child",
                                             "latent_interest_2_parent-child"};
  EXPECT_EQ(expected_top_segments,
            GetTopChildLatentInterestSegments(BuildUserModel()));
}

TEST_F(BraveAdsTopUserModelSegmentsTest, GetTopParentLatentInterestSegments) {
  // Act & Assert
  const SegmentList expected_top_segments = {"latent_interest_1_parent",
                                             "latent_interest_2_parent"};
  EXPECT_EQ(expected_top_segments,
            GetTopParentLatentInterestSegments(BuildUserModel()));
}

TEST_F(BraveAdsTopUserModelSegmentsTest, GetTopChildInterestSegments) {
  // Act & Assert
  const SegmentList expected_top_segments = {"interest_1_parent-child",
                                             "interest_2_parent-child"};
  EXPECT_EQ(expected_top_segments,
            GetTopChildInterestSegments(BuildUserModel()));
}

TEST_F(BraveAdsTopUserModelSegmentsTest, GetTopParentInterestSegments) {
  // Act & Assert
  const SegmentList expected_top_segments = {"interest_1_parent",
                                             "interest_2_parent"};
  EXPECT_EQ(expected_top_segments,
            GetTopParentInterestSegments(BuildUserModel()));
}

}  // namespace brave_ads
