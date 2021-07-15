/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/gemini/post_balance/post_balance_gemini.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/gemini/gemini_utils.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

using std::placeholders::_1;

namespace ledger {
namespace endpoint {
namespace gemini {

PostBalance::PostBalance(LedgerImpl* ledger) : ledger_(ledger) {
  DCHECK(ledger_);
}

PostBalance::~PostBalance() = default;

std::string PostBalance::GetUrl() {
  return GetApiServerUrl("/v1/balances");
}

type::Result PostBalance::ParseBody(const std::string& body,
                                    double* available) {
  DCHECK(available);

  absl::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_list()) {
    BLOG(0, "Invalid JSON");
    return type::Result::LEDGER_ERROR;
  }

  base::ListValue* balances = nullptr;
  if (!value->GetAsList(&balances)) {
    BLOG(0, "Invalid JSON");
    return type::Result::LEDGER_ERROR;
  }

  for (auto&& balance : balances->GetList()) {
    const auto* currency_code = balance.FindStringKey("currency");
    if (!currency_code || *currency_code != "BAT") {
      continue;
    }

    const auto* available_value = balance.FindStringKey("available");
    if (!available_value) {
      BLOG(0, "Missing available");
      return type::Result::LEDGER_ERROR;
    }

    const bool result =
        base::StringToDouble(base::StringPiece(*available_value), available);
    if (!result) {
      BLOG(0, "Invalid balance");
      return type::Result::LEDGER_ERROR;
    }

    return type::Result::LEDGER_OK;
  }

  // If BAT is not found in the list, BAT balance for gemini is 0
  *available = 0;
  return type::Result::LEDGER_OK;
}

void PostBalance::Request(const std::string& token,
                          PostBalanceCallback callback) {
  auto url_callback = std::bind(&PostBalance::OnRequest, this, _1, callback);
  auto request = type::UrlRequest::New();
  request->url = GetUrl();
  request->method = type::UrlMethod::POST;
  request->headers = RequestAuthorization(token);
  ledger_->LoadURL(std::move(request), url_callback);
}

void PostBalance::OnRequest(const type::UrlResponse& response,
                            PostBalanceCallback callback) {
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

}  // namespace gemini
}  // namespace endpoint
}  // namespace ledger
