/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PROMOTION_PROMOTION_TRANSFER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PROMOTION_PROMOTION_TRANSFER_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_rewards/core/ledger_callbacks.h"

namespace ledger {
class LedgerImpl;

namespace credential {
class CredentialsPromotion;
}

namespace promotion {

class PromotionTransfer {
 public:
  explicit PromotionTransfer(LedgerImpl* ledger);
  ~PromotionTransfer();

  void Start(ledger::PostSuggestionsClaimCallback callback) const;

 private:
  void OnGetSpendableUnblindedTokens(
      ledger::PostSuggestionsClaimCallback callback,
      std::vector<mojom::UnblindedTokenPtr> tokens) const;

  void OnDrainTokens(ledger::PostSuggestionsClaimCallback callback,
                     double transfer_amount,
                     mojom::Result result,
                     std::string drain_id) const;

  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<credential::CredentialsPromotion> credentials_;
};

}  // namespace promotion
}  // namespace ledger

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PROMOTION_PROMOTION_TRANSFER_H_
