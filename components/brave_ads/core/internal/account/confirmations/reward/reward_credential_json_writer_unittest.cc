/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_credential_json_writer.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_test_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_info.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_test_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsRewardCredentialJsonWriterTest : public test::TestBase {};

TEST_F(BraveAdsRewardCredentialJsonWriterTest, WriteRewardCredential) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);
  test::RefillConfirmationTokens(/*count=*/1);

  std::optional<ConfirmationInfo> confirmation =
      test::BuildRewardConfirmation(/*should_generate_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  const RewardInfo reward = test::BuildReward(*confirmation);

  // Act
  std::optional<std::string> reward_credential =
      json::writer::WriteRewardCredential(
          reward,
          /*payload=*/"definition: the weight of a payload");
  ASSERT_TRUE(reward_credential);

  // Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"JSON(
                    {
                      "signature": "jo8LVg8FpHtLKMakT7WcfGulrCB6ttSJpAaMcD95pDKTcdBRVf1yUDEZmhLKXWTIPDCDRCPKEx3gChO4S5GtHw==",
                      "t": "/mfTAAjHrWmAlLiEktbqNS/dxoMVdnz1esoVplQUs7yG/apAq2K6OeST6lBTKFJmOq7rV8QbY/DF2HFRMcz/JQ=="
                    })JSON"),
            base::test::ParseJsonDict(*reward_credential));
}

}  // namespace brave_ads
