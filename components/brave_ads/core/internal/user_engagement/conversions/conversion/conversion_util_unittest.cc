/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_util.h"

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_unittest_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsConversionInfoUtilTest, ConversionTypeToString) {
  // Arrange
  const ConversionInfo conversion;

  // Act & Assert
  EXPECT_EQ("conversion", ConversionTypeToString(conversion));
}

TEST(BraveAdsConversionInfoUtilTest, VerifiableConversionTypeToString) {
  // Arrange
  ConversionInfo conversion;
  conversion.verifiable = {kVerifiableConversionId,
                           kVerifiableConversionAdvertiserPublicKey};

  // Act & Assert
  EXPECT_EQ("verifiable conversion", ConversionTypeToString(conversion));
}

}  // namespace brave_ads
