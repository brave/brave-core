/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/promotion/get_wallet/get_wallet.h"

#include <utility>

#include "base/json/json_reader.h"
#include "bat/ledger/internal/endpoint/promotion/promotions_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace ledger {
namespace endpoint {
namespace promotion {

GetWallet::GetWallet(LedgerImpl* ledger) : ledger_{ledger} {
  DCHECK(ledger_);
}

GetWallet::~GetWallet() = default;

void GetWallet::Request(GetWalletCallback callback) const {
  auto request = type::UrlRequest::New();
  request->url = GetUrl();
  ledger_->LoadURL(std::move(request),
                   std::bind(&GetWallet::OnRequest, this, _1, callback));
}

std::string GetWallet::GetUrl() const {
  const auto rewards_wallet = ledger_->wallet()->GetWallet();
  if (!rewards_wallet) {
    BLOG(0, "Rewards wallet is null!");
    return "";
  }

  return GetServerUrl("/v3/wallet/" + rewards_wallet->payment_id);
}

void GetWallet::OnRequest(const type::UrlResponse& response,
                          GetWalletCallback callback) const {
  ledger::LogUrlResponse(__func__, response);

  type::Result result = CheckStatusCode(response.status_code);
  if (result != type::Result::LEDGER_OK) {
    return callback(result, std::string{}, false);
  }

  std::string custodian = "";
  bool linked = false;
  result = ParseBody(response.body, &custodian, &linked);
  callback(result, custodian, linked);
}

type::Result GetWallet::CheckStatusCode(int status_code) const {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid payment id");
    return type::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Unrecognized payment id");
    return type::Result::LEDGER_ERROR;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return type::Result::LEDGER_ERROR;
  }

  return type::Result::LEDGER_OK;
}

type::Result GetWallet::ParseBody(const std::string& body,
                                  std::string* custodian,
                                  bool* linked) const {
  DCHECK(custodian);
  DCHECK(linked);
  *custodian = "";
  *linked = false;

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

  if (const auto* deposit_account_provider =
          dictionary->FindDictKey("depositAccountProvider")) {
    const std::string* name = deposit_account_provider->FindStringKey("name");
    const std::string* id = deposit_account_provider->FindStringKey("id");
    const std::string* linking_id =
        deposit_account_provider->FindStringKey("linkingId");

    if (!name || !id || !linking_id) {
      return type::Result::LEDGER_ERROR;
    }

    *custodian = *name;
    *linked = !id->empty() && !linking_id->empty();
  }

  return type::Result::LEDGER_OK;
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
