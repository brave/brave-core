/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "bat/ledger/internal/contribution/contribution_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/promotion/promotion_transfer.h"
#include "bat/ledger/internal/promotion/promotion_util.h"
#include "bat/ledger/internal/request/promotion_requests.h"
#include "bat/ledger/internal/request/request_util.h"
#include "bat/ledger/internal/state_keys.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_promotion {

PromotionTransfer::PromotionTransfer(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger) {
  DCHECK(ledger_);
}

PromotionTransfer::~PromotionTransfer() = default;

void PromotionTransfer::Start(
    ledger::ExternalWalletPtr wallet,
    ledger::ResultCallback callback) {
  // we only need to call old anon api once
  if (ledger_->GetBooleanState(ledger::kStateAnonTransferChecked)) {
    GetEligibleTokens(callback);
    return;
  }

  if (!wallet) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Wallet is empty";
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transfer_callback = std::bind(&PromotionTransfer::OnAnonExternalWallet,
      this,
      _1,
      callback);

  ledger_->TransferAnonToExternalWallet(
      std::move(wallet),
      transfer_callback,
      true);
}

void PromotionTransfer::OnAnonExternalWallet(
    const ledger::Result result,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Initial transfer failed";
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  ledger_->SetBooleanState(ledger::kStateAnonTransferChecked, true);
  GetEligibleTokens(callback);
}

void PromotionTransfer::GetEligibleTokens(ledger::ResultCallback callback) {
  auto tokens_callback = std::bind(&PromotionTransfer::SendTokens,
      this,
      _1,
      callback);

  ledger_->GetUnblindedTokensByPromotionType(
      GetEligiblePromotions(),
      tokens_callback);
}

void PromotionTransfer::SendTokens(
    ledger::UnblindedTokenList list,
    ledger::ResultCallback callback) {
  if (list.empty()) {
    callback(ledger::Result::LEDGER_OK);
    return;
  }

  base::Value credentials(base::Value::Type::LIST);
  std::vector<std::string> tokens_ids;

  for (auto& item : list) {
    if (!item) {
      continue;
    }

    base::Value token(base::Value::Type::DICTIONARY);
    bool success = false;
    if (ledger::is_testing) {
      success = braveledger_contribution::GenerateSuggestionMock(
          item->token_value,
          item->public_key,
          ledger_->GetPaymentId(),
          &token);
    } else {
      success = braveledger_contribution::GenerateSuggestion(
          item->token_value,
          item->public_key,
          ledger_->GetPaymentId(),
          &token);
    }

    if (!success) {
      BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
          << "Token was not generated successfully. Token: "
          << item->token_value;
      continue;
    }

    credentials.Append(std::move(token));
    tokens_ids.push_back(std::to_string(item->id));
  }

  base::Value body(base::Value::Type::DICTIONARY);
  body.SetStringKey("paymentId", ledger_->GetPaymentId());
  body.SetKey("credentials", std::move(credentials));

  std::string json;
  base::JSONWriter::Write(body, &json);

  const std::string url = braveledger_request_util::GetTransferTokens();

  auto url_callback = std::bind(&PromotionTransfer::DeleteTokens,
      this,
      _1,
      _2,
      _3,
      tokens_ids,
      std::move(callback));

  ledger::WalletInfoProperties wallet_info = ledger_->GetWalletInfo();
  const auto headers = braveledger_request_util::BuildSignHeaders(
      "post /v1/suggestions/claim",
      json,
      ledger_->GetPaymentId(),
      wallet_info.key_info_seed);

  ledger_->LoadURL(
      url,
      headers,
      json,
      "application/json; charset=utf-8",
      ledger::UrlMethod::POST,
      url_callback);
}

void PromotionTransfer::DeleteTokens(
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const std::vector<std::string>& sent_ids,
    ledger::ResultCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  ledger_->DeleteUnblindedTokens(sent_ids, callback);
}

}  // namespace braveledger_promotion
