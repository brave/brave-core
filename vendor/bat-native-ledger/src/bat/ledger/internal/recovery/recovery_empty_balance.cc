/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>
#include <vector>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/credentials/credentials_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/recovery/recovery_empty_balance.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace {

const int32_t kVersion = 1;

}  // namespace

namespace ledger {
namespace recovery {

EmptyBalance::EmptyBalance(LedgerImpl* ledger):
    ledger_(ledger),
    promotion_server_(std::make_unique<endpoint::PromotionServer>(ledger)) {
  DCHECK(ledger_);
}

EmptyBalance::~EmptyBalance() = default;

void EmptyBalance::Check() {
  auto get_callback = std::bind(&EmptyBalance::OnAllContributions, this, _1);
  ledger_->database()->GetAllContributions(get_callback);
}

void EmptyBalance::OnAllContributions(type::ContributionInfoList list) {
  // we can just restore all tokens if no contributions
  if (list.empty()) {
    auto get_callback = std::bind(
        &EmptyBalance::GetCredsByPromotions,
        this,
        _1);

    GetPromotions(get_callback);
    return;
  }

  double contribution_sum = 0.0;
  for (const auto& contribution : list) {
    if (contribution->step == type::ContributionStep::STEP_COMPLETED) {
      contribution_sum += contribution->amount;
    }
  }

  BLOG(1, "Contribution SUM: " << contribution_sum);

  auto get_callback = std::bind(&EmptyBalance::GetAllTokens,
    this,
    _1,
    contribution_sum);

  GetPromotions(get_callback);
}

void EmptyBalance::GetPromotions(client::GetPromotionListCallback callback) {
  auto get_callback = std::bind(&EmptyBalance::OnPromotions,
    this,
    _1,
    callback);

  ledger_->database()->GetAllPromotions(get_callback);
}

void EmptyBalance::OnPromotions(
    type::PromotionMap promotions,
    client::GetPromotionListCallback callback) {
  type::PromotionList list;

  for (auto& promotion : promotions) {
    if (!promotion.second) {
      continue;
    }

    if (promotion.second->status == type::PromotionStatus::FINISHED &&
        promotion.second->type == type::PromotionType::ADS) {
      list.push_back(std::move(promotion.second));
    }
  }

  callback(std::move(list));
}

void EmptyBalance::GetCredsByPromotions(type::PromotionList list) {
  std::vector<std::string> promotion_ids;
  for (auto& promotion : list) {
    promotion_ids.push_back(promotion->id);
  }

  auto get_callback = std::bind(&EmptyBalance::OnCreds, this, _1);

  ledger_->database()->GetCredsBatchesByTriggers(promotion_ids, get_callback);
}

void EmptyBalance::OnCreds(type::CredsBatchList list) {
  if (list.empty()) {
    BLOG(1, "Creds batch list is emtpy");
    ledger_->state()->SetEmptyBalanceChecked(true);
    return;
  }

  std::string error;
  std::vector<std::string> unblinded_encoded_creds;
  type::UnblindedTokenList token_list;
  type::UnblindedTokenPtr unblinded;
  const uint64_t expires_at = 0ul;
  for (auto& creds_batch : list) {
    unblinded_encoded_creds.clear();
    error = "";
    bool result = credential::UnBlindCreds(
        *creds_batch,
        &unblinded_encoded_creds,
        &error);

    if (!result) {
      BLOG(0, "UnBlindTokens: " << error);
      continue;
    }

    for (auto& cred : unblinded_encoded_creds) {
      unblinded = type::UnblindedToken::New();
      unblinded->token_value = cred;
      unblinded->public_key = creds_batch->public_key;
      unblinded->value = 0.25;
      unblinded->creds_id = creds_batch->creds_id;
      unblinded->expires_at = expires_at;
      token_list.push_back(std::move(unblinded));
    }
  }

  if (token_list.empty()) {
    BLOG(1, "Unblinded token list is emtpy");
    ledger_->state()->SetEmptyBalanceChecked(true);
    return;
  }

  auto save_callback = std::bind(&EmptyBalance::OnSaveUnblindedCreds, this, _1);

  ledger_->database()->SaveUnblindedTokenList(
      std::move(token_list),
      save_callback);
}

void EmptyBalance::OnSaveUnblindedCreds(const type::Result result) {
  BLOG(1, "Finished empty balance migration with result: " << result);
  ledger_->state()->SetEmptyBalanceChecked(true);
}

void EmptyBalance::GetAllTokens(
    type::PromotionList list,
    const double contribution_sum) {
    // from all completed promotions get creds
    // unblind them and save them
  double promotion_sum = 0.0;
  for (auto& promotion : list) {
    promotion_sum += promotion->approximate_value;
  }

  BLOG(1, "Promotion SUM: " << promotion_sum);

  auto tokens_callback = std::bind(&EmptyBalance::ReportResults,
      this,
      _1,
      contribution_sum,
      promotion_sum);

  ledger_->database()->GetSpendableUnblindedTokensByBatchTypes(
      {type::CredsBatchType::PROMOTION},
      tokens_callback);
}

void EmptyBalance::ReportResults(
    type::UnblindedTokenList list,
    const double contribution_sum,
    const double promotion_sum) {
  double tokens_sum = 0.0;
  for (auto & item : list) {
    tokens_sum+=item->value;
  }
  BLOG(1, "Token SUM: " << tokens_sum);

  double total = promotion_sum - contribution_sum - tokens_sum;

  if (total <= 0) {
    BLOG(1, "Unblinded token total is OK");
    ledger_->state()->SetEmptyBalanceChecked(true);
    return;
  }

  BLOG(1, "Unblinded token total is " << total);

  auto url_callback = std::bind(&EmptyBalance::Sent, this, _1);

  promotion_server_->post_bat_loss()->Request(
      total,
      kVersion,
      url_callback);
}

void EmptyBalance::Sent(const type::Result result) {
  if (result != type::Result::LEDGER_OK) {
    return;
  }

  BLOG(1, "Finished empty balance migration!");
  ledger_->state()->SetEmptyBalanceChecked(true);
}

}  // namespace recovery
}  // namespace ledger
