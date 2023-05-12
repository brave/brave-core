/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PROMOTION_PROMOTION_TRANSFER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PROMOTION_PROMOTION_TRANSFER_H_

#include <string>
#include <vector>

#include "brave/components/brave_rewards/core/credentials/credentials_promotion.h"
#include "brave/components/brave_rewards/core/ledger_callbacks.h"

namespace brave_rewards::internal::promotion {

class PromotionTransfer {
 public:
  void Start(PostSuggestionsClaimCallback callback);

 private:
  void OnGetSpendableUnblindedTokens(
      PostSuggestionsClaimCallback callback,
      std::vector<mojom::UnblindedTokenPtr> tokens);

  void OnDrainTokens(PostSuggestionsClaimCallback callback,
                     double transfer_amount,
                     mojom::Result result,
                     std::string drain_id) const;

  credential::CredentialsPromotion credentials_;
};

}  // namespace brave_rewards::internal::promotion

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PROMOTION_PROMOTION_TRANSFER_H_
