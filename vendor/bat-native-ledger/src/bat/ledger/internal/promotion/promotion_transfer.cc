/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/internal/constants.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/logging/event_log_keys.h"
#include "bat/ledger/internal/promotion/promotion_transfer.h"
#include "bat/ledger/internal/promotion/promotion_util.h"
#include "bat/ledger/option_keys.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace ledger {
namespace promotion {

PromotionTransfer::PromotionTransfer(LedgerImpl* ledger) : ledger_(ledger) {
  DCHECK(ledger_);
  credentials_ = std::make_unique<credential::CredentialsPromotion>(ledger_);
  DCHECK(credentials_);
}

PromotionTransfer::~PromotionTransfer() = default;

void PromotionTransfer::GetAmount(
    ledger::GetTransferableAmountCallback callback) {
  GetEligibleTokens([callback](type::UnblindedTokenList list) {
    double amount = 0.0;
    for (const auto& item : list) {
      amount += item->value;
    }
    callback(amount);
  });
}

void PromotionTransfer::GetEligibleTokens(GetEligibleTokensCallback callback) {
  auto tokens_callback = std::bind(&PromotionTransfer::OnGetEligiblePromotions,
                                   this, _1, callback);

  ledger_->database()->GetPromotionListByType(GetEligiblePromotions(),
                                              tokens_callback);
}

void PromotionTransfer::OnGetEligiblePromotions(
    type::PromotionList promotions,
    GetEligibleTokensCallback callback) {
  std::vector<std::string> ids;
  for (auto& promotion : promotions) {
    if (!promotion) {
      continue;
    }

    ids.push_back(promotion->id);
  }

  ledger_->database()->GetSpendableUnblindedTokensByTriggerIds(
      ids,
      callback);
}

void PromotionTransfer::Start(ledger::PostSuggestionsClaimCallback callback) {
  auto tokens_callback =
      std::bind(&PromotionTransfer::OnGetEligibleTokens, this, _1, callback);

  GetEligibleTokens(tokens_callback);
}

void PromotionTransfer::OnGetEligibleTokens(
    type::UnblindedTokenList list,
    ledger::PostSuggestionsClaimCallback callback) {
  std::vector<type::UnblindedToken> token_list;
  for (auto& item : list) {
    token_list.push_back(*item);
  }

  if (token_list.empty()) {
    callback(type::Result::LEDGER_OK, "");
    return;
  }

  credential::CredentialsRedeem redeem;
  redeem.type = type::RewardsType::TRANSFER;
  redeem.processor = type::ContributionProcessor::BRAVE_TOKENS;
  redeem.token_list = token_list;

  const double transfer_amount = token_list.size() * constant::kVotePrice;
  credentials_->DrainTokens(
      redeem, [this, transfer_amount, callback](const type::Result result,
                                                std::string drain_id) {
        if (result == type::Result::LEDGER_OK) {
          ledger_->database()->SaveEventLog(log::kPromotionsClaimed,
                                            std::to_string(transfer_amount));
        }
        callback(result, drain_id);
      });
}

std::vector<type::PromotionType> PromotionTransfer::GetEligiblePromotions() {
  std::vector<type::PromotionType> promotions = {
    type::PromotionType::ADS
  };

  const bool claim_ugp =
      ledger_->ledger_client()->GetBooleanOption(option::kClaimUGP);
  if (claim_ugp) {
    promotions.push_back(type::PromotionType::UGP);
  }

  return promotions;
}

}  // namespace promotion
}  // namespace ledger
