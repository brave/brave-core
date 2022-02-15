/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/bitflyer/get_balance/get_balance_bitflyer.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/bitflyer/bitflyer_utils.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace ledger {
namespace endpoint {
namespace bitflyer {

GetBalance::GetBalance(LedgerImpl* ledger) : ledger_(ledger) {
  DCHECK(ledger_);
}

GetBalance::~GetBalance() = default;

std::string GetBalance::GetUrl() {
  return GetServerUrl("/api/link/v1/account/inventory");
}

type::Result GetBalance::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_UNAUTHORIZED ||
      status_code == net::HTTP_NOT_FOUND ||
      status_code == net::HTTP_FORBIDDEN) {
    BLOG(0, "Invalid authorization HTTP status: " << status_code);
    return type::Result::EXPIRED_TOKEN;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return type::Result::LEDGER_ERROR;
  }

  return type::Result::LEDGER_OK;
}

type::Result GetBalance::ParseBody(const std::string& body, double* available) {
  DCHECK(available);

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

  const auto* inventory = dictionary->FindListKey("inventory");
  if (!inventory) {
    BLOG(0, "Missing inventory");
    return type::Result::LEDGER_ERROR;
  }

  for (const auto& item : inventory->GetListDeprecated()) {
    const auto* currency_code = item.FindStringKey("currency_code");
    if (!currency_code || *currency_code != "BAT") {
      continue;
    }

    const auto available_value = item.FindDoubleKey("available");
    if (!available_value) {
      BLOG(0, "Missing available");
      return type::Result::LEDGER_ERROR;
    }

    *available = available_value.value();

    return type::Result::LEDGER_OK;
  }

  BLOG(0, "Missing BAT in inventory");
  return type::Result::LEDGER_ERROR;
}

void GetBalance::Request(const std::string& token,
                         GetBalanceCallback callback) {
  auto url_callback = std::bind(&GetBalance::OnRequest, this, _1, callback);
  auto request = type::UrlRequest::New();
  request->url = GetUrl();
  request->headers = RequestAuthorization(token);
  ledger_->LoadURL(std::move(request), url_callback);
}

void GetBalance::OnRequest(const type::UrlResponse& response,
                           GetBalanceCallback callback) {
  ledger::LogUrlResponse(__func__, response);

  type::Result result = CheckStatusCode(response.status_code);

  if (result != type::Result::LEDGER_OK) {
    callback(result, 0.0);
    return;
  }

  double available;
  result = ParseBody(response.body, &available);
  callback(result, available);
}

}  // namespace bitflyer
}  // namespace endpoint
}  // namespace ledger
