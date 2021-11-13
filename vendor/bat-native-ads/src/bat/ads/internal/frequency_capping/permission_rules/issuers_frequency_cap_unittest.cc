/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/issuers_frequency_cap.h"

#include "bat/ads/internal/tokens/issuers/issuers_info.h"
#include "bat/ads/internal/tokens/issuers/issuers_unittest_util.h"
#include "bat/ads/internal/tokens/issuers/issuers_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsIssuersFrequencyCapTest : public UnitTestBase {
 protected:
  BatAdsIssuersFrequencyCapTest() = default;

  ~BatAdsIssuersFrequencyCapTest() override = default;
};

TEST_F(BatAdsIssuersFrequencyCapTest, Issuers) {
  // Arrange
  const IssuersInfo& issuers = BuildIssuers(
      7200000, {{"JsvJluEN35bJBgJWTdW/8dAgPrrTM1I1pXga+o7cllo=", 0.0}},
      {{"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0}});

  SetIssuers(issuers);

  // Act
  IssuersFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsIssuersFrequencyCapTest, NoIssuers) {
  // Arrange

  // Act
  IssuersFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

}  // namespace ads
