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

using std::placeholders::_1;
using std::placeholders::_2;

namespace ledger {
namespace promotion {

PromotionTransfer::PromotionTransfer(LedgerImpl* ledger)
    : ledger_((DCHECK(ledger), ledger)),
      credentials_(
          std::make_unique<credential::CredentialsPromotion>(ledger_)) {}

PromotionTransfer::~PromotionTransfer() = default;

void PromotionTransfer::Start(
    ledger::PostSuggestionsClaimCallback callback) const {
  ledger_->database()->GetSpendableUnblindedTokens(
      std::bind(&PromotionTransfer::OnGetSpendableUnblindedTokens, this, _1,
                std::move(callback)));
}

void PromotionTransfer::OnGetSpendableUnblindedTokens(
    type::UnblindedTokenList tokens,
    ledger::PostSuggestionsClaimCallback callback) const {
  std::vector<type::UnblindedToken> token_list;
  for (auto& token : tokens) {
    token_list.push_back(*token);
  }

  if (token_list.empty()) {
    return callback(type::Result::LEDGER_OK, "");
  }

  credential::CredentialsRedeem redeem;
  redeem.type = type::RewardsType::TRANSFER;
  redeem.processor = type::ContributionProcessor::BRAVE_TOKENS;
  redeem.token_list = std::move(token_list);

  credentials_->DrainTokens(
      redeem, std::bind(&PromotionTransfer::OnDrainTokens, this, _1, _2,
                        redeem.token_list.size() * constant::kVotePrice,
                        std::move(callback)));
}

void PromotionTransfer::OnDrainTokens(
    type::Result result,
    std::string drain_id,
    double transfer_amount,
    ledger::PostSuggestionsClaimCallback callback) const {
  if (result == type::Result::LEDGER_OK) {
    ledger_->database()->SaveEventLog(log::kPromotionVBATDrained,
                                      base::NumberToString(transfer_amount));
  }

  callback(result, drain_id);
}

}  // namespace promotion
}  // namespace ledger
