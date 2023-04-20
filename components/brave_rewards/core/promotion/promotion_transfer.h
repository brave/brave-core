/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PROMOTION_PROMOTION_TRANSFER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PROMOTION_PROMOTION_TRANSFER_H_

#include <string>
#include <vector>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/core/credentials/credentials_promotion.h"
#include "brave/components/brave_rewards/core/ledger_callbacks.h"

namespace ledger {
class LedgerImpl;

namespace promotion {

class PromotionTransfer {
 public:
  explicit PromotionTransfer(LedgerImpl& ledger);
  ~PromotionTransfer();

  void Start(ledger::PostSuggestionsClaimCallback callback);

 private:
  void OnGetSpendableUnblindedTokens(
      ledger::PostSuggestionsClaimCallback callback,
      std::vector<mojom::UnblindedTokenPtr> tokens);

  void OnDrainTokens(ledger::PostSuggestionsClaimCallback callback,
                     double transfer_amount,
                     mojom::Result result,
                     std::string drain_id) const;

  const raw_ref<LedgerImpl> ledger_;
  credential::CredentialsPromotion credentials_;
};

}  // namespace promotion
}  // namespace ledger

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PROMOTION_PROMOTION_TRANSFER_H_
