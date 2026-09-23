/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue_database_table_util.h"

#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/test/transaction_test_constants.h"
#include "brave/components/brave_ads/core/internal/ad_units/test/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/test/challenge_bypass_ristretto_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_all_unit_tests --filter=BraveAds*

namespace brave_ads::database::table {

namespace {

mojom::DBRowInfoPtr BuildRewardConfirmationQueueItemRow(
    const std::string& transaction_id,
    const std::string& creative_instance_id,
    const std::string& confirmation_type,
    const std::string& ad_type,
    base::Time created_at,
    const std::string& token,
    const std::string& blinded_token,
    const std::string& unblinded_token,
    const std::string& public_key,
    const std::string& signature,
    const std::string& credential_base64url,
    const std::string& user_data_fixed,
    base::Time process_at,
    int retry_count) {
  auto mojom_db_row = mojom::DBRowInfo::New();
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(transaction_id));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(creative_instance_id));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(confirmation_type));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(ad_type));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewTimeValue(created_at));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(token));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(blinded_token));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(unblinded_token));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(public_key));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(signature));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(credential_base64url));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(user_data_fixed));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewTimeValue(process_at));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewIntValue(retry_count));
  return mojom_db_row;
}

mojom::DBRowInfoPtr BuildNonRewardConfirmationQueueItemRow(
    const std::string& transaction_id,
    const std::string& creative_instance_id,
    const std::string& confirmation_type,
    const std::string& ad_type,
    base::Time created_at,
    const std::string& user_data_fixed,
    base::Time process_at,
    int retry_count) {
  return BuildRewardConfirmationQueueItemRow(
      transaction_id, creative_instance_id, confirmation_type, ad_type,
      created_at, /*token=*/"", /*blinded_token=*/"", /*unblinded_token=*/"",
      /*public_key=*/"", /*signature=*/"", /*credential_base64url=*/"",
      user_data_fixed, process_at, retry_count);
}

}  // namespace

class BraveAdsConfirmationQueueDatabaseTableUtilTest : public ::testing::Test {
};

TEST_F(BraveAdsConfirmationQueueDatabaseTableUtilTest,
       MapAllFieldsFromMojomRow) {
  // Arrange
  const base::Time created_at =
      base::Time::FromSecondsSinceUnixEpoch(1'000'000.0);
  const base::Time process_at =
      base::Time::FromSecondsSinceUnixEpoch(2'000'000.0);
  mojom::DBRowInfoPtr mojom_db_row = BuildRewardConfirmationQueueItemRow(
      test::kTransactionId, test::kCreativeInstanceId,
      /*confirmation_type=*/"view", /*ad_type=*/"ad_notification", created_at,
      cbr::test::kTokenBase64, cbr::test::kBlindedTokenBase64,
      cbr::test::kUnblindedTokenBase64, cbr::test::kPublicKeyBase64,
      cbr::test::kVerificationSignatureBase64, "credential-base64url",
      R"({"key":"value"})", process_at, /*retry_count=*/3);

  RewardInfo expected_reward;
  expected_reward.token = cbr::Token(cbr::test::kTokenBase64);
  expected_reward.blinded_token =
      cbr::BlindedToken(cbr::test::kBlindedTokenBase64);
  expected_reward.unblinded_token =
      cbr::UnblindedToken(cbr::test::kUnblindedTokenBase64);
  expected_reward.public_key = cbr::PublicKey(cbr::test::kPublicKeyBase64);
  expected_reward.signature = cbr::test::kVerificationSignatureBase64;
  expected_reward.credential_base64url = "credential-base64url";

  ConfirmationInfo expected_confirmation;
  expected_confirmation.transaction_id = test::kTransactionId;
  expected_confirmation.creative_instance_id = test::kCreativeInstanceId;
  expected_confirmation.type = mojom::ConfirmationType::kViewedImpression;
  expected_confirmation.ad_type = mojom::AdType::kNotificationAd;
  expected_confirmation.created_at = created_at;
  expected_confirmation.reward = expected_reward;
  expected_confirmation.user_data.fixed =
      base::test::ParseJsonDict(R"({"key":"value"})");

  ConfirmationQueueItemInfo expected_confirmation_queue_item;
  expected_confirmation_queue_item.confirmation = expected_confirmation;
  expected_confirmation_queue_item.process_at = process_at;
  expected_confirmation_queue_item.retry_count = 3;

  // Act
  const ConfirmationQueueItemInfo confirmation_queue_item =
      ConfirmationQueueItemFromMojomRow(mojom_db_row);

  // Assert
  EXPECT_EQ(expected_confirmation_queue_item, confirmation_queue_item);
}

TEST_F(BraveAdsConfirmationQueueDatabaseTableUtilTest,
       NoRewardWhenRewardFieldsAreEmpty) {
  // Arrange
  const base::Time created_at =
      base::Time::FromSecondsSinceUnixEpoch(1'000'000.0);
  mojom::DBRowInfoPtr mojom_db_row = BuildNonRewardConfirmationQueueItemRow(
      test::kTransactionId, test::kCreativeInstanceId,
      /*confirmation_type=*/"view", /*ad_type=*/"ad_notification", created_at,
      /*user_data_fixed=*/"{}", /*process_at=*/base::Time(), /*retry_count=*/0);

  // Act
  const ConfirmationQueueItemInfo confirmation_queue_item =
      ConfirmationQueueItemFromMojomRow(mojom_db_row);

  // Assert
  EXPECT_FALSE(confirmation_queue_item.confirmation.reward);
}

TEST_F(BraveAdsConfirmationQueueDatabaseTableUtilTest,
       NoCreatedAtOrProcessAtWhenNull) {
  // Arrange
  mojom::DBRowInfoPtr mojom_db_row = BuildNonRewardConfirmationQueueItemRow(
      test::kTransactionId, test::kCreativeInstanceId,
      /*confirmation_type=*/"view", /*ad_type=*/"ad_notification",
      /*created_at=*/base::Time(), /*user_data_fixed=*/"{}",
      /*process_at=*/base::Time(), /*retry_count=*/0);

  // Act
  const ConfirmationQueueItemInfo confirmation_queue_item =
      ConfirmationQueueItemFromMojomRow(mojom_db_row);

  // Assert
  EXPECT_FALSE(confirmation_queue_item.confirmation.created_at);
  EXPECT_FALSE(confirmation_queue_item.process_at);
}

}  // namespace brave_ads::database::table
