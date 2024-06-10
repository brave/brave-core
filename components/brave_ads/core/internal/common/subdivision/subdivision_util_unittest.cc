/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/subdivision/subdivision_util.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsSubdivisionUtilTest, GetSubdivisionCountryCode) {
  // Act & Assert
  EXPECT_EQ("US", GetSubdivisionCountryCode(/*subdivision=*/"US-CA"));
}

TEST(BraveAdsSubdivisionUtilTest, DoNotGetSubdivisionCountryCode) {
  // Act & Assert
  EXPECT_FALSE(GetSubdivisionCountryCode(/*subdivision=*/""));
}

TEST(BraveAdsSubdivisionUtilTest, GetSubdivisionCode) {
  // Act & Assert
  EXPECT_EQ("CA", GetSubdivisionCode(/*subdivision=*/"US-CA"));
}

TEST(BraveAdsSubdivisionUtilTest, DoNotGetSubdivisionCode) {
  // Act & Assert
  EXPECT_FALSE(GetSubdivisionCode(/*subdivision=*/""));
}

}  // namespace brave_ads
