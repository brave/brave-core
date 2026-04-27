/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_database_table_util.h"

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/test/challenge_bypass_ristretto_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_all_unit_tests --filter=BraveAds*

namespace brave_ads::database::table {

namespace {

mojom::DBRowInfoPtr BuildConfirmationTokenRow(
    const std::string& unblinded_token_base64,
    const std::string& public_key_base64,
    const std::string& signature_base64) {
  auto mojom_db_row = mojom::DBRowInfo::New();
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(unblinded_token_base64));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(public_key_base64));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(signature_base64));
  return mojom_db_row;
}

}  // namespace

class BraveAdsConfirmationTokensDatabaseTableUtilTest : public ::testing::Test {
};

TEST_F(BraveAdsConfirmationTokensDatabaseTableUtilTest,
       MapAllFieldsFromMojomRow) {
  // Arrange
  mojom::DBRowInfoPtr mojom_db_row = BuildConfirmationTokenRow(
      cbr::test::kUnblindedTokenBase64, cbr::test::kPublicKeyBase64,
      cbr::test::kVerificationSignatureBase64);

  // Act
  const ConfirmationTokenInfo confirmation_token =
      ConfirmationTokenFromMojomRow(mojom_db_row);

  // Assert
  EXPECT_THAT(confirmation_token,
              ::testing::FieldsAre(
                  cbr::UnblindedToken(cbr::test::kUnblindedTokenBase64),
                  cbr::PublicKey(cbr::test::kPublicKeyBase64),
                  cbr::test::kVerificationSignatureBase64));
}

}  // namespace brave_ads::database::table
