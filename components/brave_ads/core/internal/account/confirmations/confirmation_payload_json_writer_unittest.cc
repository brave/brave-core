/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_payload_json_writer.h"

#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/token_generator_mock.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/token_generator_unittest_util.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_tokens_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

using ::testing::NiceMock;

class BraveAdsConfirmationPayloadJsonWriterTest : public UnitTestBase {
 protected:
  NiceMock<privacy::TokenGeneratorMock> token_generator_mock_;
};

TEST_F(BraveAdsConfirmationPayloadJsonWriterTest, WriteJson) {
  // Arrange
  MockTokenGenerator(token_generator_mock_, /*count*/ 1);

  privacy::SetUnblindedTokens(/*count*/ 1);

  const absl::optional<ConfirmationInfo> confirmation =
      BuildConfirmation(&token_generator_mock_);
  ASSERT_TRUE(confirmation);

  // Act
  const std::string json =
      json::writer::WriteConfirmationPayload(*confirmation);

  // Assert
  EXPECT_EQ(
      R"({"blindedPaymentTokens":["Ev5JE4/9TZI/5TqyN9JWfJ1To0HBwQw2rWeAPcdjX3Q="],"creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=","transactionId":"8b742869-6e4a-490c-ac31-31b49130098a","type":"view"})",
      json);
}

}  // namespace brave_ads
