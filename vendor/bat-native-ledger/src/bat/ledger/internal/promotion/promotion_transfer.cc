/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/promotion/promotion_transfer.h"

#include <utility>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "bat/ledger/internal/constants.h"
#include "bat/ledger/internal/credentials/credentials_promotion.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/logging/event_log_keys.h"

namespace ledger {
namespace promotion {

PromotionTransfer::PromotionTransfer(LedgerImpl* ledger)
    : ledger_(ledger),
      credentials_(
          std::make_unique<credential::CredentialsPromotion>(ledger_)) {
  DCHECK(ledger_);
}

PromotionTransfer::~PromotionTransfer() = default;

void PromotionTransfer::Start(
    ledger::PostSuggestionsClaimCallback callback) const {
  auto tokens_callback =
      base::BindOnce(&PromotionTransfer::OnGetSpendableUnblindedTokens,
                     base::Unretained(this), std::move(callback));

  ledger_->database()->GetSpendableUnblindedTokens(
      [callback = std::make_shared<decltype(tokens_callback)>(std::move(
           tokens_callback))](std::vector<mojom::UnblindedTokenPtr> tokens) {
        std::move(*callback).Run(std::move(tokens));
      });
}

void PromotionTransfer::OnGetSpendableUnblindedTokens(
    ledger::PostSuggestionsClaimCallback callback,
    std::vector<mojom::UnblindedTokenPtr> tokens) const {
  std::vector<mojom::UnblindedToken> token_list;
  for (auto& token : tokens) {
    token_list.push_back(*token);
  }

  if (token_list.empty()) {
    return std::move(callback).Run(mojom::Result::LEDGER_OK, "");
  }

  credential::CredentialsRedeem redeem;
  redeem.type = mojom::RewardsType::TRANSFER;
  redeem.processor = mojom::ContributionProcessor::BRAVE_TOKENS;
  redeem.token_list = std::move(token_list);

  credentials_->DrainTokens(
      redeem, base::BindOnce(&PromotionTransfer::OnDrainTokens,
                             base::Unretained(this), std::move(callback),
                             redeem.token_list.size() * constant::kVotePrice));
}

void PromotionTransfer::OnDrainTokens(
    ledger::PostSuggestionsClaimCallback callback,
    double transfer_amount,
    mojom::Result result,
    std::string drain_id) const {
  if (result == mojom::Result::LEDGER_OK) {
    ledger_->database()->SaveEventLog(log::kPromotionVBATDrained,
                                      base::NumberToString(transfer_amount));
  }

  std::move(callback).Run(result, std::move(drain_id));
}

}  // namespace promotion
}  // namespace ledger
