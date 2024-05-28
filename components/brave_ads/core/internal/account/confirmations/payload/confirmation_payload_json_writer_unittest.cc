/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/payload/confirmation_payload_json_writer.h"

#include <optional>

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/non_reward/non_reward_confirmation_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/non_reward/non_reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/confirmation_user_data_builder_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_mock.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_converter_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConfirmationPayloadJsonWriterTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    MockConfirmationUserData();

    AdvanceClockTo(TimeFromUTCString("Mon, 8 Jul 1996 09:25"));
  }

  TokenGeneratorMock token_generator_mock_;
};

TEST_F(BraveAdsConfirmationPayloadJsonWriterTest,
       WriteRewardConfirmationPayload) {
  // Arrange
  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  test::RefillConfirmationTokens(/*count=*/1);

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildRewardConfirmation(&token_generator_mock_,
                                    /*should_use_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  // Act
  const std::string confirmation_payload =
      json::writer::WriteConfirmationPayload(*confirmation);

  // Assert
  const base::Value::Dict expected_dict = base::test::ParseJsonDict(
      R"(
          {
            "blindedPaymentTokens": [
              "Ev5JE4/9TZI/5TqyN9JWfJ1To0HBwQw2rWeAPcdjX3Q="
            ],
            "buildChannel": "release",
            "catalog": [
              {
                "id": "29e5c8bc0ba319069980bb390d8e8f9b58c05a20"
              }
            ],
            "countryCode": "US",
            "createdAtTimestamp": "1996-07-08T09:00:00.000Z",
            "creativeInstanceId": "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
            "diagnosticId": "c1298fde-7fdb-401f-a3ce-0b58fe86e6e2",
            "platform": "windows",
            "publicKey": "RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=",
            "rotatingHash": "jBdiJH7Hu3wj31WWNLjKV5nVxFxWSDWkYh5zXCS3rXY=",
            "segment": "untargeted",
            "studies": [],
            "systemTimestamp": "1996-07-08T09:00:00.000Z",
            "topSegment": [],
            "transactionId": "8b742869-6e4a-490c-ac31-31b49130098a",
            "type": "view",
            "versionNumber": "1.2.3.4"
          })");
  EXPECT_EQ(expected_dict, base::test::ParseJsonDict(confirmation_payload));
}

TEST_F(BraveAdsConfirmationPayloadJsonWriterTest,
       WriteNonRewardConfirmationPayload) {
  // Arrange
  test::DisableBraveRewards();

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildNonRewardConfirmation(/*should_use_random_uuids=*/false);
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
