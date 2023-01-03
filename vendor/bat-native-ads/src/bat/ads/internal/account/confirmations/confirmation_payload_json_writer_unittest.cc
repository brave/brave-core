/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/confirmations/confirmation_payload_json_writer.h"

#include "bat/ads/internal/account/confirmations/confirmation_unittest_util.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_tokens_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kExpectedJson[] =
    R"({"blindedPaymentTokens":["Ev5JE4/9TZI/5TqyN9JWfJ1To0HBwQw2rWeAPcdjX3Q="],"creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=","transactionId":"8b742869-6e4a-490c-ac31-31b49130098a","type":"view"})";

}  // namespace

class BatAdsConfirmationPayloadJsonWriterTest : public UnitTestBase {};

TEST_F(BatAdsConfirmationPayloadJsonWriterTest, WriteJson) {
  // Arrange
  privacy::SetUnblindedTokens(1);
  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  const std::string json =
      json::writer::WriteConfirmationPayload(*confirmation);

  // Assert
  EXPECT_EQ(kExpectedJson, json);
}

}  // namespace ads
