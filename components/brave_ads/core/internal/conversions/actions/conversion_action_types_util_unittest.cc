/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/actions/conversion_action_types_util.h"

#include "brave/components/brave_ads/core/public/confirmation_type.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsConversionActionTypesUtilTest, ToViewThroughConversionActionType) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(ConversionActionType::kViewThrough,
            ToConversionActionType(ConfirmationType::kViewed));
}

TEST(BraveAdsConversionActionTypesUtilTest,
     ToClickThroughConversionActionType) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(ConversionActionType::kClickThrough,
            ToConversionActionType(ConfirmationType::kClicked));
}

TEST(BraveAdsConversionActionTypesUtilTest,
     StringToViewThroughConversionActionType) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(ConversionActionType::kViewThrough,
            StringToConversionActionType("view"));
}

TEST(BraveAdsConversionActionTypesUtilTest,
     StringToClickThroughConversionActionType) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(ConversionActionType::kClickThrough,
            StringToConversionActionType("click"));
}

TEST(BraveAdsConversionActionTypesUtilTest,
     ViewThroughConversionActionTypeToString) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ("view",
            ConversionActionTypeToString(ConversionActionType::kViewThrough));
}

TEST(BraveAdsConversionActionTypesUtilTest,
     ClickThroughConversionActionTypeToString) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ("click",
            ConversionActionTypeToString(ConversionActionType::kClickThrough));
}

}  // namespace brave_ads
