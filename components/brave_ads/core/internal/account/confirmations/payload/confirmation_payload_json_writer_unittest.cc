/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/payload/confirmation_payload_json_writer.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/non_reward/non_reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_mock.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConfirmationPayloadJsonWriterTest : public UnitTestBase {
 protected:
  TokenGeneratorMock token_generator_mock_;
};

TEST_F(BraveAdsConfirmationPayloadJsonWriterTest,
       WriteRewardConfirmationPayload) {
  // Arrange
  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  test::SetConfirmationTokens(/*count=*/1);

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids=*/false);
  const absl::optional<ConfirmationInfo> confirmation = BuildRewardConfirmation(
      &token_generator_mock_, transaction, /*user_data=*/{});
  ASSERT_TRUE(confirmation);

  // Act
  const std::string confirmation_payload =
      json::writer::WriteConfirmationPayload(*confirmation);

  // Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"(
                    {
                      "blindedPaymentTokens": [
                        "Ev5JE4/9TZI/5TqyN9JWfJ1To0HBwQw2rWeAPcdjX3Q="
                      ],
                      "creativeInstanceId": "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
                      "publicKey": "RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=",
                      "transactionId": "8b742869-6e4a-490c-ac31-31b49130098a",
                      "type": "view"
                    })"),
            base::test::ParseJsonDict(confirmation_payload));
}

TEST_F(BraveAdsConfirmationPayloadJsonWriterTest,
       WriteNonRewardConfirmationPayload) {
  // Arrange
  test::DisableBraveRewards();

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids=*/false);
  const absl::optional<ConfirmationInfo> confirmation =
      BuildNonRewardConfirmation(transaction,
                                 /*user_data=*/{});
  ASSERT_TRUE(confirmation);

  // Act
  const std::string confirmation_payload =
      json::writer::WriteConfirmationPayload(*confirmation);

  // Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"(
                    {
                      "creativeInstanceId": "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
                      "transactionId": "8b742869-6e4a-490c-ac31-31b49130098a",
                      "type": "view"
                    })"),
            base::test::ParseJsonDict(confirmation_payload));
}

}  // namespace brave_ads
