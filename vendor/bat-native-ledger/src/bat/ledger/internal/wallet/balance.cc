/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet/balance.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/uphold/uphold.h"
#include "bat/ledger/internal/rapidjson_bat_helper.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_wallet {

Balance::Balance(bat_ledger::LedgerImpl* ledger) :
    uphold_(std::make_unique<braveledger_uphold::Uphold>(ledger)),
    ledger_(ledger) {
}

Balance::~Balance() {
}

void Balance::Fetch(ledger::FetchBalanceCallback callback) {
  std::string payment_id = ledger_->GetPaymentId();

  std::string path = (std::string)WALLET_PROPERTIES
      + payment_id
      + WALLET_PROPERTIES_END;
  const std::string url = braveledger_bat_helper::buildURL(
      path,
      PREFIX_V2,
      braveledger_bat_helper::SERVER_TYPES::BALANCE);
  auto load_callback = std::bind(&Balance::OnWalletProperties,
                            this,
                            _1,
                            _2,
                            _3,
                            callback);
  ledger_->LoadURL(url,
                   std::vector<std::string>(),
                   std::string(),
                   std::string(),
                   ledger::UrlMethod::GET,
                   load_callback);
}

void Balance::OnWalletProperties(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    ledger::FetchBalanceCallback callback) {
  ledger::BalancePtr balance = ledger::Balance::New();
  ledger_->LogResponse(__func__, response_status_code, response, headers);
  if (response_status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR, std::move(balance));
    return;
  }

  base::Optional<base::Value> value = base::JSONReader::Read(response);
  if (!value || !value->is_dict()) {
    callback(ledger::Result::LEDGER_ERROR, std::move(balance));
    return;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    callback(ledger::Result::LEDGER_ERROR, std::move(balance));
    return;
  }

  auto* total = dictionary->FindKey("balance");
  double total_anon = 0.0;
  if (total) {
    total_anon = std::stod(total->GetString());
  }
  balance->total = total_anon;

  auto* funds = dictionary->FindKey("cardBalance");
  std::string user_funds = "0";
  if (funds) {
    user_funds = funds->GetString();
  }
  balance->user_funds = user_funds;

  auto* local_rates = dictionary->FindKey("rates");
  if (local_rates) {
    base::DictionaryValue* dict_value = nullptr;
    if (local_rates->GetAsDictionary(&dict_value)) {
      for (const auto& it : dict_value->DictItems()) {
        balance->rates.insert(std::make_pair(it.first, it.second.GetDouble()));
      }
    }
  }

  balance->wallets.insert(std::make_pair(ledger::kWalletAnonymous, total_anon));

  // Fetch other wallets
  auto tokens_callback = std::bind(&Balance::OnExternalWallets,
                                   this,
                                   *balance,
                                   callback,
                                   _1);

  ledger_->GetExternalWallets(tokens_callback);
}

void Balance::OnExternalWallets(
    ledger::Balance info,
    ledger::FetchBalanceCallback callback,
    std::map<std::string, ledger::ExternalWalletPtr> wallets) {
  if (wallets.size() == 0) {
    ledger::BalancePtr info_ptr = ledger::Balance::New(info);
    callback(ledger::Result::LEDGER_OK, std::move(info_ptr));
    return;
  }

  auto uphold_callback = std::bind(&Balance::OnUpholdFetchBalance,
                                   this,
                                   info,
                                   callback,
                                   _1,
                                   _2);

  uphold_->FetchBalance(std::move(wallets), uphold_callback);
}

void Balance::OnUpholdFetchBalance(ledger::Balance info,
                                   ledger::FetchBalanceCallback callback,
                                   ledger::Result result,
                                   double balance) {
  ledger::BalancePtr info_ptr = ledger::Balance::New(info);

  if (result == ledger::Result::LEDGER_ERROR) {
    callback(ledger::Result::LEDGER_ERROR, std::move(info_ptr));
    return;
  }

  info_ptr->wallets.insert(std::make_pair(ledger::kWalletUphold, balance));
  info_ptr->total += balance;
  callback(ledger::Result::LEDGER_OK, std::move(info_ptr));
}

// static
double Balance::GetPerWalletBalance(
    const std::string& type,
    base::flat_map<std::string, double> wallets) {
  if (type.empty() || wallets.size() == 0) {
    return 0.0;
  }

  for (const auto& wallet : wallets) {
    if (wallet.first == type) {
      return wallet.second;
    }
  }

  return  0.0;
}

}  // namespace braveledger_wallet
