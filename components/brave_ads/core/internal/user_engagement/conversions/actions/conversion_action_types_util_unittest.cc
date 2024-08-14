/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/actions/conversion_action_types_util.h"

#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsConversionActionTypesUtilTest, ToViewThroughConversionActionType) {
  // Act & Assert
  EXPECT_EQ(ConversionActionType::kViewThrough,
            ToConversionActionType(ConfirmationType::kViewedImpression));
}

TEST(BraveAdsConversionActionTypesUtilTest,
     ToClickThroughConversionActionType) {
  // Act & Assert
  EXPECT_EQ(ConversionActionType::kClickThrough,
            ToConversionActionType(ConfirmationType::kClicked));
}

TEST(BraveAdsConversionActionTypesUtilTest,
     StringToViewThroughConversionActionType) {
  // Act & Assert
  EXPECT_EQ(ConversionActionType::kViewThrough, ToConversionActionType("view"));
}

TEST(BraveAdsConversionActionTypesUtilTest,
     StringToClickThroughConversionActionType) {
  // Act & Assert
  EXPECT_EQ(ConversionActionType::kClickThrough,
            ToConversionActionType("click"));
}

TEST(BraveAdsConversionActionTypesUtilTest,
     ViewThroughConversionActionTypeToString) {
  // Act & Assert
  EXPECT_EQ("view", ToString(ConversionActionType::kViewThrough));
}

TEST(BraveAdsConversionActionTypesUtilTest,
     ClickThroughConversionActionTypeToString) {
  // Act & Assert
  EXPECT_EQ("click", ToString(ConversionActionType::kClickThrough));
}

}  // namespace brave_ads
