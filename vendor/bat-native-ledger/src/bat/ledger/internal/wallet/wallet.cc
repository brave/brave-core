/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet/wallet.h"

#include <utility>

#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/rapidjson_bat_helper.h"
#include "bat/ledger/internal/wallet/balance.h"
#include "bat/ledger/internal/wallet/create.h"
#include "bat/ledger/internal/wallet/recover.h"
#include "bat/ledger/internal/uphold/uphold.h"
#include "mojo/public/cpp/bindings/map.h"
#include "net/http/http_status_code.h"

#include "wally_bip39.h"  // NOLINT

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_wallet {

Wallet::Wallet(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger),
    create_(std::make_unique<Create>(ledger)),
    recover_(std::make_unique<Recover>(ledger)),
    balance_(std::make_unique<Balance>(ledger)),
    uphold_(std::make_unique<braveledger_uphold::Uphold>(ledger)) {
}

Wallet::~Wallet() {
}

void Wallet::CreateWalletIfNecessary() {
  const auto payment_id = ledger_->GetPaymentId();
  const auto stamp = ledger_->GetBootStamp();
  const auto persona_id = ledger_->GetPersonaId();

  if (!payment_id.empty() && stamp != 0 && !persona_id.empty()) {
    ledger_->OnWalletInitialized(ledger::Result::WALLET_CREATED);
    return;
  }

  BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
     "Wallet creation didn't finish or corrupted. " <<
     "We need to clear persona Id and start again";
  ledger_->SetPersonaId("");

  create_->Start();
}

void Wallet::GetWalletProperties(
    ledger::OnWalletPropertiesCallback callback) {
  std::string payment_id = ledger_->GetPaymentId();
  std::string passphrase = GetWalletPassphrase();

  if (payment_id.empty() || passphrase.empty()) {
    braveledger_bat_helper::WALLET_PROPERTIES_ST properties;
    ledger_->OnWalletProperties(ledger::Result::CORRUPTED_WALLET, properties);
    return;
  }

  std::string path = (std::string)WALLET_PROPERTIES
      + payment_id
      + WALLET_PROPERTIES_END;
  const std::string url = braveledger_bat_helper::buildURL(
      path,
      PREFIX_V2,
      braveledger_bat_helper::SERVER_TYPES::BALANCE);
  auto load_callback = std::bind(&Wallet::WalletPropertiesCallback,
                            this,
                            _1,
                            _2,
                            _3,
                            callback);
  ledger_->LoadURL(url,
                   std::vector<std::string>(),
                   std::string(),
                   std::string(),
                   ledger::URL_METHOD::GET,
                   load_callback);
}

ledger::WalletPropertiesPtr Wallet::WalletPropertiesToWalletInfo(
    const braveledger_bat_helper::WALLET_PROPERTIES_ST& properties) {
  ledger::WalletPropertiesPtr wallet = ledger::WalletProperties::New();
  wallet->parameters_choices = properties.parameters_choices_;
  wallet->fee_amount = ledger_->GetContributionAmount();
  wallet->parameters_range = properties.parameters_range_;
  wallet->parameters_days = properties.parameters_days_;

  for (size_t i = 0; i < properties.grants_.size(); i ++) {
    ledger::GrantPtr grant = ledger::Grant::New();

    grant->altcurrency = properties.grants_[i].altcurrency;
    grant->probi = properties.grants_[i].probi;
    grant->expiry_time = properties.grants_[i].expiryTime;
    grant->type = properties.grants_[i].type;

    wallet->grants.push_back(std::move(grant));
  }

  return wallet;
}

void Wallet::WalletPropertiesCallback(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    ledger::OnWalletPropertiesCallback callback) {
  braveledger_bat_helper::WALLET_PROPERTIES_ST properties;
  ledger_->LogResponse(__func__, response_status_code, response, headers);
  if (response_status_code != net::HTTP_OK) {
    ledger_->OnWalletProperties(ledger::Result::LEDGER_ERROR, properties);
    return;
  }

  ledger::WalletPropertiesPtr wallet;

  bool ok = braveledger_bat_helper::loadFromJson(&properties, response);

  if (!ok) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "Failed to load wallet properties state";
    callback(ledger::Result::LEDGER_ERROR, std::move(wallet));
    return;
  }

  wallet = WalletPropertiesToWalletInfo(properties);

  ledger_->SetWalletProperties(&properties);
  callback(ledger::Result::LEDGER_OK, std::move(wallet));
}

std::string Wallet::GetWalletPassphrase() const {
  braveledger_bat_helper::WALLET_INFO_ST wallet_info = ledger_->GetWalletInfo();
  std::string passPhrase;
  if (wallet_info.keyInfoSeed_.size() == 0) {
    return passPhrase;
  }

  char* words = nullptr;
  int result = bip39_mnemonic_from_bytes(nullptr,
                                         &wallet_info.keyInfoSeed_.front(),
                                         wallet_info.keyInfoSeed_.size(),
                                         &words);
  if (result != 0) {
    DCHECK(false);

    return passPhrase;
  }
  passPhrase = words;
  wally_free_string(words);

  return passPhrase;
}

void Wallet::RecoverWallet(const std::string& pass_phrase) {
  recover_->Start(pass_phrase);
}

void Wallet::GetAddressesForPaymentId(
    ledger::WalletAddressesCallback callback) {
  std::string currency = ledger_->GetCurrency();
  std::string path = (std::string)WALLET_PROPERTIES +
                      ledger_->GetPaymentId() +
                      "?refresh=true";
  ledger_->LoadURL(
      braveledger_bat_helper::buildURL(path, PREFIX_V2),
      std::vector<std::string>(),
      "",
      "",
      ledger::URL_METHOD::GET,
      std::bind(&Wallet::GetAddressesForPaymentIdCallback,
                this,
                _1,
                _2,
                _3,
                callback));
}

void Wallet::GetAddressesForPaymentIdCallback(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    ledger::WalletAddressesCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  std::map<std::string, std::string> addresses;
  bool ok = braveledger_bat_helper::getJSONAddresses(response, &addresses);

  if (!ok) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
       "Failed to get addresses from payment ID";
    return;
  }

  callback(addresses);
  ledger_->SetAddresses(addresses);
}

void Wallet::FetchBalance(ledger::FetchBalanceCallback callback) {
  balance_->Fetch(callback);
}

void Wallet::OnGetExternalWallet(
    const std::string& type,
    ledger::ExternalWalletCallback callback,
    std::map<std::string, ledger::ExternalWalletPtr> wallets) {
  if (type == ledger::kWalletUphold) {
    ledger::ExternalWalletPtr wallet;
    if (wallets.size() == 0) {
      wallet = ledger::ExternalWallet::New();
      wallet->status = ledger::WalletStatus::NOT_CONNECTED;
    } else {
      wallet = uphold_->GetWallet(std::move(wallets));
    }

    wallet->verify_url = uphold_->GetVerifyUrl();
    callback(std::move(wallet));
    return;
  }

  callback(nullptr);
}

void Wallet::GetExternalWallet(const std::string& type,
                               ledger::ExternalWalletCallback callback) {
  auto wallet_callback = std::bind(&Wallet::OnGetExternalWallet,
                                   this,
                                   type,
                                   callback,
                                   _1);

  ledger_->GetExternalWallets(wallet_callback);
}

}  // namespace braveledger_wallet
