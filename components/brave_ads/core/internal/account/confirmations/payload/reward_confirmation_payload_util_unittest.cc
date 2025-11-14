/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/payload/reward_confirmation_payload_util.h"

#include <optional>

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_test_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_test_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsRewardConfirmationPayloadUtilTest : public test::TestBase {};

TEST_F(BraveAdsRewardConfirmationPayloadUtilTest,
       BuildRewardConfirmationPayload) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);
  test::RefillConfirmationTokens(/*count=*/1);

  std::optional<ConfirmationInfo> confirmation =
      test::BuildRewardConfirmation(/*should_generate_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  const RewardInfo reward = test::BuildReward(*confirmation);

  // Act &s Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"JSON(
                    {
                      "blindedPaymentTokens": [
                        "+qJiMi6k0hRzRAEN239nLthLqrNm53O78x/PV8I/JS0="
                      ],
                      "publicKey": "OqhZpUC8B15u+Gc11rQYRl8O3zOSAUIEC2JuDHI32TM="
                    })JSON"),
            BuildRewardConfirmationPayload(reward));
}

}  // namespace brave_ads
