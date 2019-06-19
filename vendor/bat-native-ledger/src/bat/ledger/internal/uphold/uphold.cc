/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "bat/ledger/internal/uphold/uphold.h"
#include "bat/ledger/internal/uphold/uphold_contribution.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_uphold {

Uphold::Uphold(bat_ledger::LedgerImpl* ledger) :
    contribution_(std::make_unique<UpholdContribution>(ledger)),
    ledger_(ledger) {
}

Uphold::~Uphold() {
}

void Uphold::StartContribution(const std::string &viewing_id,
                               ledger::ExternalWallet wallet) {
  contribution_->Start(viewing_id, wallet);
}

void Uphold::FetchBalance(
    std::map<std::string, ledger::ExternalWallet> wallets,
    FetchBalanceCallback callback) {
  const ledger::ExternalWallet wallet = GetWallet(wallets);

  if (wallet.token.empty()) {
    callback(ledger::Result::LEDGER_ERROR, 0.0);
    return;
  }

  std::vector<std::string> headers;
  headers.push_back("Authorization: Bearer " + wallet.token);
  const std::string url = GetAPIUrl("/v0/me/cards/" + wallet.address);

  auto balance_callback = std::bind(&Uphold::OnFetchBalance,
                                    this,
                                    callback,
                                    _1,
                                    _2,
                                    _3);

  ledger_->LoadURL(
      url,
      headers,
      "",
      "",
      ledger::URL_METHOD::GET,
      balance_callback);
}

void Uphold::OnFetchBalance(
    FetchBalanceCallback callback,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);
  if (response_status_code == net::HTTP_UNAUTHORIZED ||
      response_status_code == net::HTTP_NOT_FOUND) {
    // TODO add token expiration flow
    return;
  }

  if (response_status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR, 0.0);
    return;
  }

  base::Optional<base::Value> value = base::JSONReader::Read(response);
  if (!value || !value->is_dict()) {
    callback(ledger::Result::LEDGER_ERROR, 0.0);
    return;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    callback(ledger::Result::LEDGER_ERROR, 0.0);
    return;
  }

  auto* available = dictionary->FindKey("available");
  if (available) {
    callback(ledger::Result::LEDGER_OK, std::stod(available->GetString()));
    return;
  }

  callback(ledger::Result::LEDGER_ERROR, 0.0);
}

ledger::ExternalWallet Uphold::GetWallet(
    std::map<std::string, ledger::ExternalWallet> wallets) {
  for (const auto& wallet : wallets) {
    if (wallet.first == ledger::kWalletUphold) {
      return wallet.second;
    }
  }

  ledger::ExternalWallet empty;
  return empty;
}

std::string Uphold::GetAPIUrl(const std::string& path) {
  std::string url;
  if (ledger::is_production) {
    url = kUrlProduction;
  } else {
    url = kUrlStaging;
  }

  return url + path;
}

}  // namespace braveledger_uphold
