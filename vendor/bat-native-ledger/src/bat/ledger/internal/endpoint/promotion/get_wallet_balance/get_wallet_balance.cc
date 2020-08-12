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

// GET /v3/wallet/uphold/{payment_id}
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_BAD_REQUEST (400)
// HTTP_NOT_FOUND (404)
// HTTP_SERVICE_UNAVAILABLE (503)
//
// Response body:
// {
//  "total": 0.0
//  "spendable": 0.0
//  "confirmed": 0.0
//  "unconfirmed": 0.0
// }

namespace ledger {
namespace endpoint {
namespace promotion {

GetWalletBalance::GetWalletBalance(bat_ledger::LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

GetWalletBalance::~GetWalletBalance() = default;

std::string GetWalletBalance::GetUrl() {
  const std::string payment_id = ledger_->state()->GetPaymentId();
  const std::string path = base::StringPrintf(
      "/v3/wallet/uphold/%s",
      payment_id.c_str());

  return GetServerUrl(path);
}

ledger::Result GetWalletBalance::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid payment id");
    return ledger::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Unrecognized payment id");
    return ledger::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_SERVICE_UNAVAILABLE) {
    BLOG(0, "No conversion rate yet in ratios service");
    return ledger::Result::LEDGER_ERROR;
  }

  if (status_code != net::HTTP_OK) {
    return ledger::Result::LEDGER_ERROR;
  }

  return ledger::Result::LEDGER_OK;
}

ledger::Result GetWalletBalance::ParseBody(
    const std::string& body,
    ledger::Balance* balance) {
  DCHECK(balance);

  base::Optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  const auto confirmed = dictionary->FindDoubleKey("confirmed");
  if (confirmed) {
    balance->total = *confirmed;
  }

  balance->user_funds = balance->total;
  balance->wallets.insert(
      std::make_pair(ledger::kWalletAnonymous, balance->total));

  return ledger::Result::LEDGER_OK;
}

void GetWalletBalance::Request(GetWalletBalanceCallback callback) {
  auto url_callback = std::bind(&GetWalletBalance::OnRequest,
      this,
      _1,
      callback);
  ledger_->LoadURL(
      GetUrl(),
      {},
      "",
      "",
      ledger::UrlMethod::GET,
      url_callback);
}

void GetWalletBalance::OnRequest(
    const ledger::UrlResponse& response,
    GetWalletBalanceCallback callback) {
  ledger::LogUrlResponse(__func__, response);

  ledger::Result result = CheckStatusCode(response.status_code);

  if (result != ledger::Result::LEDGER_OK) {
    callback(result, nullptr);
    return;
  }

  ledger::Balance balance;
  result = ParseBody(response.body, &balance);
  callback(result, ledger::Balance::New(balance));
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
