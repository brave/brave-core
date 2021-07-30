/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/get_wallet_balance/get_wallet_balance.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/endpoint/promotion/promotions_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace ledger {
namespace endpoint {
namespace promotion {

GetWalletBalance::GetWalletBalance(LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

GetWalletBalance::~GetWalletBalance() = default;

std::string GetWalletBalance::GetUrl() {
  const auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    return "";
  }

  const std::string path = base::StringPrintf(
      "/v3/wallet/uphold/%s",
      wallet->payment_id.c_str());

  return GetServerUrl(path);
}

type::Result GetWalletBalance::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid payment id");
    return type::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Unrecognized payment id");
    return type::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_SERVICE_UNAVAILABLE) {
    BLOG(0, "No conversion rate yet in ratios service");
    return type::Result::LEDGER_ERROR;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return type::Result::LEDGER_ERROR;
  }

  return type::Result::LEDGER_OK;
}

type::Result GetWalletBalance::ParseBody(
    const std::string& body,
    type::Balance* balance) {
  DCHECK(balance);

  absl::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return type::Result::LEDGER_ERROR;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Invalid JSON");
    return type::Result::LEDGER_ERROR;
  }

  const auto confirmed = dictionary->FindDoubleKey("confirmed");
  if (confirmed) {
    balance->total = *confirmed;
  }

  balance->user_funds = balance->total;
  balance->wallets.insert(
      std::make_pair(constant::kWalletAnonymous, balance->total));

  return type::Result::LEDGER_OK;
}

void GetWalletBalance::Request(GetWalletBalanceCallback callback) {
  auto url_callback = std::bind(&GetWalletBalance::OnRequest,
      this,
      _1,
      callback);
  auto request = type::UrlRequest::New();
  request->url = GetUrl();
  ledger_->LoadURL(std::move(request), url_callback);
}

void GetWalletBalance::OnRequest(
    const type::UrlResponse& response,
    GetWalletBalanceCallback callback) {
  ledger::LogUrlResponse(__func__, response);

  type::Result result = CheckStatusCode(response.status_code);

  if (result != type::Result::LEDGER_OK) {
    callback(result, nullptr);
    return;
  }

  type::Balance balance;
  result = ParseBody(response.body, &balance);
  callback(result, type::Balance::New(balance));
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
