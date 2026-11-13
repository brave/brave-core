/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/renderer/order_id_extractor_util.h"

#include "testing/gtest/include/gtest/gtest.h"

// pnpm test brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsOrderIdExtractorUtilTest, ValidateIdWithNoValidationPattern) {
  // Act & Assert
  EXPECT_EQ("ABC12345",
            MaybeValidateId("ABC12345", /*validation_pattern=*/std::nullopt));
}

TEST(BraveAdsOrderIdExtractorUtilTest,
     ValidateIdWithZeroGroupPatternThatFullyMatches) {
  // Act & Assert
  EXPECT_EQ("ABC12345", MaybeValidateId("ABC12345", "^[A-Z0-9]{8}$"));
}

TEST(BraveAdsOrderIdExtractorUtilTest,
     DoesNotValidateIdWithZeroGroupPatternThatOnlyPartiallyMatches) {
  // Act & Assert
  EXPECT_FALSE(MaybeValidateId("ABC12345", "ABC"));
}

TEST(BraveAdsOrderIdExtractorUtilTest,
     ValidateIdWithMultiGroupPatternReturnsEntireIdUnchanged) {
  // Act & Assert
  EXPECT_EQ("ABC12345", MaybeValidateId("ABC12345", "^([A-Z]{3})([0-9]{5})$"));
}

TEST(BraveAdsOrderIdExtractorUtilTest,
     DoesNotValidateIdIfPatternDoesNotFullyMatch) {
  // Act & Assert
  EXPECT_FALSE(MaybeValidateId("ABC1234", "^[A-Z0-9]{8}$"));
}

TEST(BraveAdsOrderIdExtractorUtilTest,
     DoesNotValidateIdWithNonCompilingValidationPattern) {
  // Act & Assert
  EXPECT_FALSE(MaybeValidateId("ABC12345", "(unterminated"));
}

}  // namespace brave_ads
