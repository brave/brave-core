/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/get_recover_wallet/get_recover_wallet.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/promotion/promotions_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace ledger {
namespace endpoint {
namespace promotion {

GetRecoverWallet::GetRecoverWallet(LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

GetRecoverWallet::~GetRecoverWallet() = default;

std::string GetRecoverWallet::GetUrl(const std::string& public_key_hex) {
  const std::string& path = base::StringPrintf(
      "/v3/wallet/recover/%s",
      public_key_hex.c_str());

  return GetServerUrl(path);
}

type::Result GetRecoverWallet::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return type::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Not found");
    return type::Result::NOT_FOUND;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return type::Result::LEDGER_ERROR;
  }

  return type::Result::LEDGER_OK;
}

type::Result GetRecoverWallet::ParseBody(
    const std::string& body,
    std::string* payment_id,
    bool* legacy_wallet) {
  DCHECK(payment_id && legacy_wallet);

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

  const auto* payment_id_string = dictionary->FindStringKey("paymentId");
  if (!payment_id_string || payment_id_string->empty()) {
    BLOG(0, "Payment id is missing");
    return type::Result::LEDGER_ERROR;
  }

  const auto* wallet_name =
      dictionary->FindStringPath("walletProvider.name");
  if (!wallet_name) {
    BLOG(0, "Wallet name is missing");
    return type::Result::LEDGER_ERROR;
  }

  *legacy_wallet = *wallet_name == "uphold";
  *payment_id = *payment_id_string;
  return type::Result::LEDGER_OK;
}

void GetRecoverWallet::Request(
    const std::string& public_key_hex,
    GetRecoverWalletCallback callback) {
  auto url_callback = std::bind(&GetRecoverWallet::OnRequest,
      this,
      _1,
      callback);

  auto request = type::UrlRequest::New();
  request->url = GetUrl(public_key_hex);
  ledger_->LoadURL(std::move(request), url_callback);
}

void GetRecoverWallet::OnRequest(
    const type::UrlResponse& response,
    GetRecoverWalletCallback callback) {
  ledger::LogUrlResponse(__func__, response);

  std::string payment_id;
  bool legacy_wallet = false;
  type::Result result = CheckStatusCode(response.status_code);

  if (result != type::Result::LEDGER_OK) {
    callback(result, payment_id, legacy_wallet);
    return;
  }

  result = ParseBody(response.body, &payment_id, &legacy_wallet);
  callback(result, payment_id, legacy_wallet);
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
