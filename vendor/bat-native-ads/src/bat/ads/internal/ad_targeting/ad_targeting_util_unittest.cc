/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/ad_targeting_util.h"

#include "bat/ads/internal/ad_targeting/ad_targeting_user_model_info.h"
#include "bat/ads/internal/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace ad_targeting {

class BatAdsAdTargetingUtilTest : public UnitTestBase {
 protected:
  BatAdsAdTargetingUtilTest() = default;

  ~BatAdsAdTargetingUtilTest() override = default;
};

TEST_F(BatAdsAdTargetingUtilTest, GetTopParentChildSegments) {
  // Arrange
  UserModelInfo user_model;
  user_model.interest_segments = {"interest-1", "interest-2"};
  user_model.latent_interest_segments = {"latent_interest-1",
                                         "latent_interest-2"};
  user_model.purchase_intent_segments = {"purchase_intent-1",
                                         "purchase_intent-2"};

  // Act
  const SegmentList segments =
      GetTopSegments(user_model, /* parent_only */ false);

  // Assert
  const SegmentList expected_segments = {
      "interest-1",        "interest-2",        "latent_interest-1",
      "latent_interest-2", "purchase_intent-1", "purchase_intent-2"};

  EXPECT_EQ(expected_segments, segments);
}

TEST_F(BatAdsAdTargetingUtilTest, GetTopParentChildSegmentsForEmptyUserModel) {
  // Arrange
  const UserModelInfo user_model;

  // Act
  const SegmentList segments =
      GetTopSegments(user_model, /* parent_only */ false);

  // Assert
  EXPECT_TRUE(segments.empty());
}

TEST_F(BatAdsAdTargetingUtilTest, GetTopParentSegments) {
  // Arrange
  UserModelInfo user_model;
  user_model.interest_segments = {"interest_1", "interest_2"};
  user_model.latent_interest_segments = {"latent_interest_1",
                                         "latent_interest_2"};
  user_model.purchase_intent_segments = {"purchase_intent_1",
                                         "purchase_intent_2"};

  // Act
  const SegmentList segments =
      GetTopSegments(user_model, /* parent_only */ true);

  // Assert
  const SegmentList expected_segments = {
      "interest_1",        "interest_2",        "latent_interest_1",
      "latent_interest_2", "purchase_intent_1", "purchase_intent_2"};

  EXPECT_EQ(expected_segments, segments);
}

TEST_F(BatAdsAdTargetingUtilTest, GetTopParentSegmentsForEmptyUserModel) {
  // Arrange
  const UserModelInfo user_model;

  // Act
  const SegmentList segments =
      GetTopSegments(user_model, /* parent_only */ true);

  // Assert
  EXPECT_TRUE(segments.empty());
}

}  // namespace ad_targeting
}  // namespace ads
