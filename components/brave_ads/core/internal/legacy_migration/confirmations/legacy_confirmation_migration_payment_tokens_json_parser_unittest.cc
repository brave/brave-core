/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/confirmations/legacy_confirmation_migration_payment_tokens_json_parser.h"

#include "base/test/task_environment.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/common/test/test_constants.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::json::reader {

class BraveAdsLegacyConfirmationMigrationPaymentTokensJsonParserTest
    : public ::testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
};

TEST_F(BraveAdsLegacyConfirmationMigrationPaymentTokensJsonParserTest,
       ParsePaymentTokens) {
  // Act
  std::optional<PaymentTokenList> payment_tokens = ParsePaymentTokens(R"JSON({
    "unblinded_payment_tokens": [
      {
        "transaction_id": "8b742869-6e4a-490c-ac31-31b49130098a",
        "unblinded_token": "IXDCnZnVEJ0orkbZfr2ut2NQPQ0ofdervKBmQ2hyjcClGCjA3/ExbBumO0ua5cxwo//nN0uKQ60kknru8hRXx0DWhwHwuFlxmot8WgVbnQ0XtPx7q9BG0jbI00AJStwN",
        "public_key": "QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g=",
        "confirmation_type": "view",
        "ad_type": "ad_notification"
      },
      {
        "transaction_id": "c4ed0916-8c4d-4731-8fd6-c9a35cc0bce5",
        "unblinded_token": "IXDCnZnVEJ0orkbZfr2ut2NQPQ0ofdervKBmQ2hyjcClGCjA3/ExbBumO0ua5cxwo//nN0uKQ60kknru8hRXx0DWhwHwuFlxmot8WgVbnQ0XtPx7q9BG0jbI00AJStwN",
        "public_key": "QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g=",
        "confirmation_type": "click",
        "ad_type": "ad_notification"
      }
    ]
  })JSON");

  // Assert
  EXPECT_THAT(
      payment_tokens,
      ::testing::Optional(::testing::ElementsAre(
          ::testing::FieldsAre(
              /*transaction_id*/ "8b742869-6e4a-490c-ac31-31b49130098a",
              cbr::UnblindedToken(
                  R"(IXDCnZnVEJ0orkbZfr2ut2NQPQ0ofdervKBmQ2hyjcClGCjA3/ExbBumO0ua5cxwo//nN0uKQ60kknru8hRXx0DWhwHwuFlxmot8WgVbnQ0XtPx7q9BG0jbI00AJStwN)"),
              cbr::PublicKey("QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g="),
              mojom::ConfirmationType::kViewedImpression,
              mojom::AdType::kNotificationAd),
          ::testing::FieldsAre(
              /*transaction_id*/ "c4ed0916-8c4d-4731-8fd6-c9a35cc0bce5",
              cbr::UnblindedToken(
                  R"(IXDCnZnVEJ0orkbZfr2ut2NQPQ0ofdervKBmQ2hyjcClGCjA3/ExbBumO0ua5cxwo//nN0uKQ60kknru8hRXx0DWhwHwuFlxmot8WgVbnQ0XtPx7q9BG0jbI00AJStwN)"),
              cbr::PublicKey("QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g="),
              mojom::ConfirmationType::kClicked,
              mojom::AdType::kNotificationAd))));
}

TEST_F(BraveAdsLegacyConfirmationMigrationPaymentTokensJsonParserTest,
       ParsePaymentTokensAndAssignTransactionIdWhenMissing) {
  // Act
  std::optional<PaymentTokenList> payment_tokens = ParsePaymentTokens(R"JSON({
    "unblinded_payment_tokens": [
      {
        "unblinded_token": "IXDCnZnVEJ0orkbZfr2ut2NQPQ0ofdervKBmQ2hyjcClGCjA3/ExbBumO0ua5cxwo//nN0uKQ60kknru8hRXx0DWhwHwuFlxmot8WgVbnQ0XtPx7q9BG0jbI00AJStwN",
        "public_key": "QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g=",
        "confirmation_type": "view",
        "ad_type": "ad_notification"
      }
    ]
  })JSON");

  // Assert
  EXPECT_THAT(payment_tokens,
              ::testing::Optional(::testing::ElementsAre(::testing::FieldsAre(
                  /*transaction_id=*/::testing::Not(::testing::IsEmpty()),
                  /*unblinded_token=*/::testing::_,
                  /*public_key=*/::testing::_,
                  /*confirmation_type=*/::testing::_,
                  /*ad_type=*/::testing::_))));
}

TEST_F(BraveAdsLegacyConfirmationMigrationPaymentTokensJsonParserTest,
       ParsePaymentTokensDropsLegacyInlineContentAdType) {
  // Act
  std::optional<PaymentTokenList> payment_tokens = ParsePaymentTokens(R"JSON({
    "unblinded_payment_tokens": [
      {
        "transaction_id": "8b742869-6e4a-490c-ac31-31b49130098a",
        "unblinded_token": "IXDCnZnVEJ0orkbZfr2ut2NQPQ0ofdervKBmQ2hyjcClGCjA3/ExbBumO0ua5cxwo//nN0uKQ60kknru8hRXx0DWhwHwuFlxmot8WgVbnQ0XtPx7q9BG0jbI00AJStwN",
        "public_key": "QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g=",
        "confirmation_type": "view",
        "ad_type": "inline_content_ad"
      }
    ]
  })JSON");

  // Assert
  EXPECT_THAT(payment_tokens, ::testing::Optional(::testing::IsEmpty()));
}

TEST_F(BraveAdsLegacyConfirmationMigrationPaymentTokensJsonParserTest,
       ParsePaymentTokensDropsLegacyPromotedContentAdType) {
  // Act
  std::optional<PaymentTokenList> payment_tokens = ParsePaymentTokens(R"JSON({
    "unblinded_payment_tokens": [
      {
        "transaction_id": "8b742869-6e4a-490c-ac31-31b49130098a",
        "unblinded_token": "IXDCnZnVEJ0orkbZfr2ut2NQPQ0ofdervKBmQ2hyjcClGCjA3/ExbBumO0ua5cxwo//nN0uKQ60kknru8hRXx0DWhwHwuFlxmot8WgVbnQ0XtPx7q9BG0jbI00AJStwN",
        "public_key": "QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g=",
        "confirmation_type": "view",
        "ad_type": "promoted_content_ad"
      }
    ]
  })JSON");

  // Assert
  EXPECT_THAT(payment_tokens, ::testing::Optional(::testing::IsEmpty()));
}

TEST_F(BraveAdsLegacyConfirmationMigrationPaymentTokensJsonParserTest,
       ParsePaymentTokensSkipsMissingUnblindedToken) {
  // Act
  std::optional<PaymentTokenList> payment_tokens = ParsePaymentTokens(R"JSON({
    "unblinded_payment_tokens": [
      {
        "transaction_id": "8b742869-6e4a-490c-ac31-31b49130098a",
        "public_key": "QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g="
      }
    ]
  })JSON");

  // Assert
  EXPECT_THAT(payment_tokens, ::testing::Optional(::testing::IsEmpty()));
}

TEST_F(BraveAdsLegacyConfirmationMigrationPaymentTokensJsonParserTest,
       ParsePaymentTokensSkipsInvalidUnblindedToken) {
  // Act
  std::optional<PaymentTokenList> payment_tokens = ParsePaymentTokens(R"JSON({
    "unblinded_payment_tokens": [
      {
        "transaction_id": "8b742869-6e4a-490c-ac31-31b49130098a",
        "unblinded_token": "INVALID_TOKEN",
        "public_key": "QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g="
      }
    ]
  })JSON");

  // Assert
  EXPECT_THAT(payment_tokens, ::testing::Optional(::testing::IsEmpty()));
}

TEST_F(BraveAdsLegacyConfirmationMigrationPaymentTokensJsonParserTest,
       ParsePaymentTokensSkipsMissingPublicKey) {
  // Act
  std::optional<PaymentTokenList> payment_tokens = ParsePaymentTokens(R"JSON({
    "unblinded_payment_tokens": [
      {
        "transaction_id": "8b742869-6e4a-490c-ac31-31b49130098a",
        "unblinded_token": "IXDCnZnVEJ0orkbZfr2ut2NQPQ0ofdervKBmQ2hyjcClGCjA3/ExbBumO0ua5cxwo//nN0uKQ60kknru8hRXx0DWhwHwuFlxmot8WgVbnQ0XtPx7q9BG0jbI00AJStwN"
      }
    ]
  })JSON");

  // Assert
  EXPECT_THAT(payment_tokens, ::testing::Optional(::testing::IsEmpty()));
}

TEST_F(BraveAdsLegacyConfirmationMigrationPaymentTokensJsonParserTest,
       ParsePaymentTokensSkipsInvalidPublicKey) {
  // Act
  std::optional<PaymentTokenList> payment_tokens = ParsePaymentTokens(R"JSON({
    "unblinded_payment_tokens": [
      {
        "transaction_id": "8b742869-6e4a-490c-ac31-31b49130098a",
        "unblinded_token": "IXDCnZnVEJ0orkbZfr2ut2NQPQ0ofdervKBmQ2hyjcClGCjA3/ExbBumO0ua5cxwo//nN0uKQ60kknru8hRXx0DWhwHwuFlxmot8WgVbnQ0XtPx7q9BG0jbI00AJStwN",
        "public_key": "INVALID_KEY"
      }
    ]
  })JSON");

  // Assert
  EXPECT_THAT(payment_tokens, ::testing::Optional(::testing::IsEmpty()));
}

TEST_F(BraveAdsLegacyConfirmationMigrationPaymentTokensJsonParserTest,
       DoNotParsePaymentTokensIfMissingUnblindedPaymentTokensKey) {
  // Act & Assert
  EXPECT_FALSE(ParsePaymentTokens(R"JSON({"other_key": []})JSON"));
}

TEST_F(BraveAdsLegacyConfirmationMigrationPaymentTokensJsonParserTest,
       DoNotParsePaymentTokensIfMalformedJson) {
  // Act & Assert
  EXPECT_FALSE(ParsePaymentTokens(test::kMalformedJson));
}

TEST_F(BraveAdsLegacyConfirmationMigrationPaymentTokensJsonParserTest,
       ParseEmptyPaymentTokens) {
  // Act & Assert
  EXPECT_THAT(ParsePaymentTokens(R"JSON({"unblinded_payment_tokens": []})JSON"),
              ::testing::Optional(::testing::IsEmpty()));
}

}  // namespace brave_ads::json::reader
