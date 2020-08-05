/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>
#include <vector>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/credentials/credentials_util.h"
#include "bat/ledger/internal/recovery/recovery_empty_balance.h"
#include "bat/ledger/internal/request/request_promotion.h"
#include "bat/ledger/internal/request/request_util.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace {

const int32_t kVersion = 1;

}  // namespace

namespace braveledger_recovery {

void EmptyBalance::Check(bat_ledger::LedgerImpl* ledger) {
  auto get_callback = std::bind(
      &EmptyBalance::OnAllContributions,
      _1,
      ledger);

  ledger->database()->GetAllContributions(get_callback);
}

void EmptyBalance::OnAllContributions(
    ledger::ContributionInfoList list,
    bat_ledger::LedgerImpl* ledger) {
  // we can just restore all tokens if no contributions
  if (list.empty()) {
    auto get_callback = std::bind(&EmptyBalance::GetCredsByPromotions,
        _1,
        ledger);

    GetPromotions(ledger, get_callback);
    return;
  }

  double contribution_sum = 0.0;
  for (const auto& contribution : list) {
    if (contribution->step == ledger::ContributionStep::STEP_COMPLETED) {
      contribution_sum += contribution->amount;
    }
  }

  BLOG(1, "Contribution SUM: " << contribution_sum);

  auto get_callback = std::bind(&EmptyBalance::GetAllTokens,
    _1,
    ledger,
    contribution_sum);

  GetPromotions(ledger, get_callback);
}

void EmptyBalance::GetPromotions(
    bat_ledger::LedgerImpl* ledger,
    ledger::GetPromotionListCallback callback) {
  auto get_callback = std::bind(&EmptyBalance::OnPromotions,
    _1,
    callback);

  ledger->database()->GetAllPromotions(get_callback);
}

void EmptyBalance::OnPromotions(
    ledger::PromotionMap promotions,
    ledger::GetPromotionListCallback callback) {
  ledger::PromotionList list;

  for (auto& promotion : promotions) {
    if (!promotion.second) {
      continue;
    }

    if (promotion.second->status == ledger::PromotionStatus::FINISHED &&
        promotion.second->type == ledger::PromotionType::ADS) {
      list.push_back(std::move(promotion.second));
    }
  }

  callback(std::move(list));
}

void EmptyBalance::GetCredsByPromotions(
    ledger::PromotionList list,
    bat_ledger::LedgerImpl* ledger) {
  std::vector<std::string> promotion_ids;
  for (auto& promotion : list) {
    promotion_ids.push_back(promotion->id);
  }

  auto get_callback = std::bind(&EmptyBalance::OnCreds,
    _1,
    ledger);

  ledger->database()->GetCredsBatchesByTriggers(promotion_ids, get_callback);
}

void EmptyBalance::OnCreds(
    ledger::CredsBatchList list,
    bat_ledger::LedgerImpl* ledger) {
  if (list.empty()) {
    BLOG(1, "Creds batch list is emtpy");
    ledger->state()->SetEmptyBalanceChecked(true);
    return;
  }

  std::string error;
  std::vector<std::string> unblinded_encoded_creds;
  ledger::UnblindedTokenList token_list;
  ledger::UnblindedTokenPtr unblinded;
  const uint64_t expires_at = 0ul;
  for (auto& creds_batch : list) {
    unblinded_encoded_creds.clear();
    error = "";
    bool result = braveledger_credentials::UnBlindCreds(
        *creds_batch,
        &unblinded_encoded_creds,
        &error);

    if (!result) {
      BLOG(0, "UnBlindTokens: " << error);
      continue;
    }

    for (auto& cred : unblinded_encoded_creds) {
      unblinded = ledger::UnblindedToken::New();
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
    ledger->state()->SetEmptyBalanceChecked(true);
    return;
  }

  auto save_callback = std::bind(&EmptyBalance::OnSaveUnblindedCreds,
      _1,
      ledger);

  ledger->database()->SaveUnblindedTokenList(
      std::move(token_list),
      save_callback);
}

void EmptyBalance::OnSaveUnblindedCreds(
    const ledger::Result result,
    bat_ledger::LedgerImpl* ledger) {
  BLOG(1, "Finished empty balance migration with result: " << result);
  ledger->state()->SetEmptyBalanceChecked(true);
}

void EmptyBalance::GetAllTokens(
    ledger::PromotionList list,
    bat_ledger::LedgerImpl* ledger,
    const double contribution_sum) {
    // from all completed promotions get creds
    // unblind them and save them
  double promotion_sum = 0.0;
  for (auto& promotion : list) {
    promotion_sum += promotion->approximate_value;
  }

  BLOG(1, "Promotion SUM: " << promotion_sum);

  auto tokens_callback = std::bind(&EmptyBalance::ReportResults,
      _1,
      ledger,
      contribution_sum,
      promotion_sum);

  ledger->database()->GetSpendableUnblindedTokensByBatchTypes(
      {ledger::CredsBatchType::PROMOTION},
      tokens_callback);
}

void EmptyBalance::ReportResults(
    ledger::UnblindedTokenList list,
    bat_ledger::LedgerImpl* ledger,
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
    ledger->state()->SetEmptyBalanceChecked(true);
    return;
  }

  BLOG(1, "Unblinded token total is " << total);

  const std::string json = base::StringPrintf(
      R"({"amount": %f})",
      total);

  const std::string payment_id = ledger->state()->GetPaymentId();
  auto url_callback = std::bind(&EmptyBalance::Sent,
      _1,
      ledger);

  const std::string header_url = base::StringPrintf(
      "post /v1/wallets/%s/events/batloss/%d",
      payment_id.c_str(),
      kVersion);

  const auto headers = braveledger_request_util::BuildSignHeaders(
      header_url,
      json,
      payment_id,
      ledger->state()->GetRecoverySeed());

  const std::string url = braveledger_request_util::GetBatlossURL(
      payment_id,
      kVersion);
  ledger->LoadURL(
      url,
      headers,
      json,
      "application/json; charset=utf-8",
      ledger::UrlMethod::POST,
      url_callback);
}

void EmptyBalance::Sent(
    const ledger::UrlResponse& response,
    bat_ledger::LedgerImpl* ledger) {
  BLOG(6, ledger::UrlResponseToString(__func__, response));

  if (response.status_code != net::HTTP_OK) {
    return;
  }

  BLOG(1, "Finished empty balance migration!");
  ledger->state()->SetEmptyBalanceChecked(true);
}

}  // namespace braveledger_recovery
