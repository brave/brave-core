/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_test_util.h"

#include <string>

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_test_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::test {

std::optional<ConfirmationInfo> BuildRewardConfirmation(
    bool should_generate_random_uuids) {
  CHECK(UserHasJoinedBraveRewards());

  const TransactionInfo transaction = BuildUnreconciledTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression, should_generate_random_uuids);
  return BuildRewardConfirmation(transaction, /*user_data=*/{});
}

std::optional<ConfirmationInfo> BuildRewardConfirmationWithoutDynamicUserData(
    bool should_generate_random_uuids) {
  std::optional<ConfirmationInfo> confirmation =
      BuildRewardConfirmation(should_generate_random_uuids);
  CHECK(confirmation);

  return RebuildConfirmationWithoutDynamicUserData(*confirmation);
}

RewardInfo BuildReward(const ConfirmationInfo& confirmation) {
  RewardInfo reward;

  reward.token = cbr::Token(
      R"(/mfTAAjHrWmAlLiEktbqNS/dxoMVdnz1esoVplQUs7yG/apAq2K6OeST6lBTKFJmOq7rV8QbY/DF2HFRMcz/JVkVTu9dLQdR595gZf/D4PvSuhgk5RcoBm3fSFGI4JQF)");

  reward.blinded_token =
      cbr::BlindedToken(R"(+qJiMi6k0hRzRAEN239nLthLqrNm53O78x/PV8I/JS0=)");

  reward.unblinded_token = cbr::UnblindedToken(
      R"(/mfTAAjHrWmAlLiEktbqNS/dxoMVdnz1esoVplQUs7yG/apAq2K6OeST6lBTKFJmOq7rV8QbY/DF2HFRMcz/JTrpqSWv/sNVO/Pi8nHDyl3CET+S2CKkMmYlXW3DgqxW)");

  reward.public_key =
      cbr::PublicKey(R"(OqhZpUC8B15u+Gc11rQYRl8O3zOSAUIEC2JuDHI32TM=)");

  reward.signature =
      R"(pWHhVf6jDdMbt2tKKk3E0JJAB7J5lGnJej/Vi9/UgQpdqw9kKBgvmj4ke0R2MP2n2ynhRjM1sRVZiez0G2hpCA==)";

  std::optional<std::string> reward_credential_base64url =
      BuildRewardCredential(confirmation);
  CHECK(reward_credential_base64url);
  reward.credential_base64url = *reward_credential_base64url;

  return reward;
}

}  // namespace brave_ads::test
