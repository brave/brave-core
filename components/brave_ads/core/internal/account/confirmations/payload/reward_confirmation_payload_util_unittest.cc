/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/payload/reward_confirmation_payload_util.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsRewardConfirmationPayloaUtilTest : public UnitTestBase {};

TEST_F(BraveAdsRewardConfirmationPayloaUtilTest,
       BuildRewardConfirmationPayload) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(
      base::test::ParseJsonDict(
          R"~({"blindedPaymentTokens":["mNRViqFD8ZPpjRZi4Xwj1UEsU1j9qPNc4R/BoiWsVi0="],"publicKey":"QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g="})~"),
      BuildRewardConfirmationPayload(BuildReward()));
}

}  // namespace brave_ads
