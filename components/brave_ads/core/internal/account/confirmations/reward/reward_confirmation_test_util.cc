/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_test_util.h"

#include <string>

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
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
    const bool should_generate_random_uuids) {
  CHECK(UserHasJoinedBraveRewards());

  const TransactionInfo transaction = BuildUnreconciledTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression, should_generate_random_uuids);
  return BuildRewardConfirmation(transaction, /*user_data=*/{});
}

RewardInfo BuildReward(const ConfirmationInfo& confirmation) {
  RewardInfo reward;

  reward.token = cbr::Token(
      R"(aXZNwft34oG2JAVBnpYh/ktTOzr2gi0lKosYNczUUz6ZS9gaDTJmU2FHFps9dIq+QoDwjSjctR5v0rRn+dYo+AHScVqFAgJ5t2s4KtSyawW10gk6hfWPQw16Q0+8u5AG)");

  reward.blinded_token =
      cbr::BlindedToken(R"(Ev5JE4/9TZI/5TqyN9JWfJ1To0HBwQw2rWeAPcdjX3Q=)");

  reward.unblinded_token = cbr::UnblindedToken(
      R"(PLowz2WF2eGD5zfwZjk9p76HXBLDKMq/3EAZHeG/fE2XGQ48jyte+Ve50ZlasOuYL5mwA8CU2aFMlJrt3DDgC3B1+VD/uyHPfa/+bwYRrpVH5YwNSDEydVx8S4r+BYVY)");

  reward.public_key =
      cbr::PublicKey(R"(RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=)");

  reward.signature =
      R"(+yxJmIDobOZ5DBncIVuzjQEZfIa0+UPrSQhzA5pwEAL9qC4UW7A1H35nKAhVLehJlXnnfMVKV02StVO3fBU5CQ==)";

  const std::optional<std::string> reward_credential_base64url =
      BuildRewardCredential(confirmation);
  CHECK(reward_credential_base64url);
  reward.credential_base64url = *reward_credential_base64url;

  return reward;
}

}  // namespace brave_ads::test
