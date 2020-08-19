/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/logging/event_log_keys.h"
#include "bat/ledger/internal/promotion/promotion_transfer.h"
#include "bat/ledger/internal/promotion/promotion_util.h"
#include "bat/ledger/internal/static_values.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_promotion {

PromotionTransfer::PromotionTransfer(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger) {
  DCHECK(ledger_);
  credentials_ = braveledger_credentials::CredentialsFactory::Create(
      ledger_,
      ledger::CredsBatchType::PROMOTION);
  DCHECK(credentials_);
}

PromotionTransfer::~PromotionTransfer() = default;

void PromotionTransfer::Start(ledger::ResultCallback callback) {
  auto tokens_callback = std::bind(&PromotionTransfer::GetEligibleTokens,
      this,
      _1,
      callback);

  ledger_->database()->GetPromotionListByType(
      GetEligiblePromotions(),
      tokens_callback);
}

void PromotionTransfer::GetEligibleTokens(
    ledger::PromotionList promotions,
    ledger::ResultCallback callback) {
  auto tokens_callback = std::bind(&PromotionTransfer::OnGetEligibleTokens,
      this,
      _1,
      callback);

  std::vector<std::string> ids;
  for (auto& promotion : promotions) {
    if (!promotion) {
      continue;
    }

    ids.push_back(promotion->id);
  }

  ledger_->database()->GetSpendableUnblindedTokensByTriggerIds(
      ids,
      tokens_callback);
}

void PromotionTransfer::OnGetEligibleTokens(
    ledger::UnblindedTokenList list,
    ledger::ResultCallback callback) {
  if (list.empty()) {
    BLOG(1, "No eligible tokens");
    callback(ledger::Result::LEDGER_OK);
    return;
  }

  std::vector<ledger::UnblindedToken> token_list;
  for (auto& item : list) {
    token_list.push_back(*item);
  }

  braveledger_credentials::CredentialsRedeem redeem;
  redeem.type = ledger::RewardsType::TRANSFER;
  redeem.processor = ledger::ContributionProcessor::BRAVE_TOKENS;
  redeem.token_list = token_list;

  const double transfer_amount =
      token_list.size() * braveledger_ledger::_vote_price;
  credentials_->RedeemTokens(
      redeem,
      [this, transfer_amount, callback](const ledger::Result result) {
        if (result == ledger::Result::LEDGER_OK) {
            ledger_->database()->SaveEventLog(
                ledger::log::kPromotionsClaimed,
                std::to_string(transfer_amount));
        }
        callback(result);
      });
}

}  // namespace braveledger_promotion
