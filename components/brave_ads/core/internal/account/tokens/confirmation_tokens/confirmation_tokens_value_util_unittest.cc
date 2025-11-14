/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_value_util.h"

#include <string_view>

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_token_info.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr std::string_view kConfirmationTokensAsJson =
    R"JSON(
        [
          {
            "public_key": "OqhZpUC8B15u+Gc11rQYRl8O3zOSAUIEC2JuDHI32TM=",
            "signature": "pWHhVf6jDdMbt2tKKk3E0JJAB7J5lGnJej/Vi9/UgQpdqw9kKBgvmj4ke0R2MP2n2ynhRjM1sRVZiez0G2hpCA==",
            "unblinded_token": "/mfTAAjHrWmAlLiEktbqNS/dxoMVdnz1esoVplQUs7yG/apAq2K6OeST6lBTKFJmOq7rV8QbY/DF2HFRMcz/JTrpqSWv/sNVO/Pi8nHDyl3CET+S2CKkMmYlXW3DgqxW"
          },
          {
            "public_key": "OqhZpUC8B15u+Gc11rQYRl8O3zOSAUIEC2JuDHI32TM=",
            "signature": "4gR1GFr60vt8qJhZtuKHwJsysnH42dqD6lCt9f12krhXnAPKxZwwQXM583bH38vBooEG5sKkRaPPQltP5/c5AQ==",
            "unblinded_token": "8o+uKml91AthZzZgdH0iVGtZHfK2lAmPFdlhfF8rXnF67OtwnzHntFOimAmE73L7d9zh2sl0TJMv0mYwFf3zfsLb7m0IE1e3UqGPVaKP3B+rLmCjbSau+mfzjqQ8OQMS"
          }
        ])JSON";

}  // namespace

class BraveAdsConfirmationTokenValueUtilTest : public test::TestBase {};

TEST_F(BraveAdsConfirmationTokenValueUtilTest, ConfirmationTokensToValue) {
  // Arrange
  const ConfirmationTokenList confirmation_tokens =
      test::BuildConfirmationTokens(/*count=*/2);

  // Act
  const base::Value::List list = ConfirmationTokensToValue(confirmation_tokens);

  // Assert
  EXPECT_EQ(base::test::ParseJsonList(kConfirmationTokensAsJson), list);
}

TEST_F(BraveAdsConfirmationTokenValueUtilTest, EmptyConfirmationTokensToValue) {
  // Act
  const base::Value::List list = ConfirmationTokensToValue({});

  // Assert
  EXPECT_THAT(list, ::testing::IsEmpty());
}

TEST_F(BraveAdsConfirmationTokenValueUtilTest, ConfirmationTokensFromValue) {
  // Arrange
  const base::Value::List list =
      base::test::ParseJsonList(kConfirmationTokensAsJson);

  // Act
  const ConfirmationTokenList confirmation_tokens =
      ConfirmationTokensFromValue(list);

  // Assert
  const ConfirmationTokenList expected_confirmation_tokens =
      test::BuildConfirmationTokens(/*count=*/2);
  EXPECT_EQ(test::BuildConfirmationTokens(/*count=*/2), confirmation_tokens);
}

TEST_F(BraveAdsConfirmationTokenValueUtilTest,
       EmptyConfirmationTokensFromValue) {
  // Act & Assert
  EXPECT_THAT(ConfirmationTokensFromValue({}), ::testing::IsEmpty());
}

}  // namespace brave_ads
