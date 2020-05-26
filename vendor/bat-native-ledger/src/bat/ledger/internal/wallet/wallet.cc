/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet/wallet.h"

#include <map>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/legacy/unsigned_tx_properties.h"
#include "bat/ledger/internal/legacy/unsigned_tx_state.h"
#include "bat/ledger/internal/legacy/wallet_info_properties.h"
#include "bat/ledger/internal/legacy/wallet_state.h"
#include "bat/ledger/internal/request/request_util.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/internal/state/state_util.h"
#include "bat/ledger/internal/uphold/uphold.h"
#include "bat/ledger/internal/wallet/balance.h"
#include "bat/ledger/internal/wallet/create.h"
#include "bat/ledger/internal/wallet/recover.h"
#include "bat/ledger/internal/wallet/wallet_util.h"
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

void Wallet::CreateWalletIfNecessary(ledger::ResultCallback callback) {
  const auto payment_id = ledger_->GetPaymentId();
  const auto stamp = ledger_->GetCreationStamp();

  if (!payment_id.empty() && stamp != 0) {
    BLOG(1, "Wallet already exists");
    callback(ledger::Result::WALLET_CREATED);
    return;
  }

  create_->Start(std::move(callback));
}

void Wallet::GetWalletProperties(ledger::OnWalletPropertiesCallback callback) {
  std::string payment_id = ledger_->GetPaymentId();
  std::string passphrase = GetWalletPassphrase();

  if (payment_id.empty() || passphrase.empty()) {
    BLOG(0, "Wallet corruption");
    ledger::WalletProperties properties;
    callback(ledger::Result::CORRUPTED_DATA,
             WalletPropertiesToWalletInfo(properties));
    return;
  }

  std::string path = (std::string)WALLET_PROPERTIES
      + payment_id
      + WALLET_PROPERTIES_END;
  const std::string url = braveledger_request_util::BuildUrl(
      path,
      PREFIX_V2,
      braveledger_request_util::ServerTypes::BALANCE);
  auto load_callback = std::bind(&Wallet::WalletPropertiesCallback,
                            this,
                            _1,
                            callback);
  ledger_->LoadURL(url, {}, "", "", ledger::UrlMethod::GET, load_callback);
}

ledger::WalletPropertiesPtr Wallet::WalletPropertiesToWalletInfo(
    const ledger::WalletProperties& properties) {
  ledger::WalletPropertiesPtr wallet = ledger::WalletProperties::New();
  wallet->parameters_choices = properties.parameters_choices;
  wallet->fee_amount = ledger_->GetAutoContributionAmount();
  wallet->default_tip_choices = properties.default_tip_choices;
  wallet->default_monthly_tip_choices = properties.default_monthly_tip_choices;

  return wallet;
}

void Wallet::WalletPropertiesCallback(
    const ledger::UrlResponse& response,
    ledger::OnWalletPropertiesCallback callback) {
  BLOG(6, ledger::UrlResponseToString(__func__, response));
  ledger::WalletProperties properties;
  if (response.status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR,
             WalletPropertiesToWalletInfo(properties));
    return;
  }

  ledger::WalletPropertiesPtr wallet;

  const ledger::WalletState wallet_state;
  bool ok = wallet_state.FromJson(response.body, &properties);

  if (!ok) {
    BLOG(0, "Failed to load wallet properties state");
    callback(ledger::Result::LEDGER_ERROR, std::move(wallet));
    return;
  }

  wallet = WalletPropertiesToWalletInfo(properties);

  ledger_->SetWalletProperties(&properties);
  callback(ledger::Result::LEDGER_OK, std::move(wallet));
}

std::string Wallet::GetWalletPassphrase() const {
  const auto seed = braveledger_state::GetRecoverySeed(ledger_);
  std::string passPhrase;
  if (seed.size() == 0) {
    return passPhrase;
  }

  char* words = nullptr;
  int result = bip39_mnemonic_from_bytes(nullptr,
                                         &seed.front(),
                                         seed.size(),
                                         &words);
  if (result != 0) {
    DCHECK(false);

    return passPhrase;
  }
  passPhrase = words;
  wally_free_string(words);

  return passPhrase;
}

void Wallet::RecoverWallet(
    const std::string& pass_phrase,
    ledger::RecoverWalletCallback callback) {
  recover_->Start(pass_phrase, std::move(callback));
}

void Wallet::FetchBalance(ledger::FetchBalanceCallback callback) {
  balance_->Fetch(callback);
}

void Wallet::OnGetExternalWallet(
    const std::string& wallet_type,
    ledger::ExternalWalletCallback callback,
    std::map<std::string, ledger::ExternalWalletPtr> wallets) {
  if (wallet_type == ledger::kWalletUphold) {
    uphold_->GenerateExternalWallet(std::move(wallets), callback);
    return;
  }

  callback(ledger::Result::LEDGER_ERROR, nullptr);
}

void Wallet::GetExternalWallet(const std::string& wallet_type,
                               ledger::ExternalWalletCallback callback) {
  auto wallet_callback = std::bind(&Wallet::OnGetExternalWallet,
                                   this,
                                   wallet_type,
                                   callback,
                                   _1);

  ledger_->GetExternalWallets(wallet_callback);
}

void Wallet::OnExternalWalletAuthorization(
    const std::string& wallet_type,
    const std::map<std::string, std::string>& args,
    ledger::ExternalWalletAuthorizationCallback callback,
    std::map<std::string, ledger::ExternalWalletPtr> wallets) {
  if (wallets.size() == 0) {
    BLOG(0, "No wallets");
    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }

  if (wallet_type == ledger::kWalletUphold) {
    uphold_->WalletAuthorization(args,
                                 std::move(wallets),
                                 callback);
  }
}

void Wallet::ExternalWalletAuthorization(
    const std::string& wallet_type,
    const std::map<std::string, std::string>& args,
    ledger::ExternalWalletAuthorizationCallback callback) {
  auto wallet_callback = std::bind(&Wallet::OnExternalWalletAuthorization,
                                   this,
                                   wallet_type,
                                   args,
                                   callback,
                                   _1);

  ledger_->GetExternalWallets(wallet_callback);
}

void Wallet::OnDisconnectWallet(
    const std::string& wallet_type,
    ledger::ResultCallback callback,
    std::map<std::string, ledger::ExternalWalletPtr> wallets) {
  if (wallets.size() == 0) {
    BLOG(0, "No wallets");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto wallet_ptr = GetWallet(wallet_type, std::move(wallets));

  if (!wallet_ptr) {
    BLOG(0, "Wallet is null");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  wallet_ptr = ResetWallet(std::move(wallet_ptr));

  ledger_->SaveExternalWallet(wallet_type, std::move(wallet_ptr));
  callback(ledger::Result::LEDGER_OK);
}

void Wallet::DisconnectWallet(
      const std::string& wallet_type,
      ledger::ResultCallback callback) {
  auto wallet_callback = std::bind(&Wallet::OnDisconnectWallet,
                                   this,
                                   wallet_type,
                                   callback,
                                   _1);

  ledger_->GetExternalWallets(wallet_callback);
}

void Wallet::OnTransferAnonToExternalWallet(
    const ledger::UrlResponse& response,
    ledger::ResultCallback callback) {
  BLOG(6, ledger::UrlResponseToString(__func__, response));

  if (response.status_code == net::HTTP_OK) {
    callback(ledger::Result::LEDGER_OK);
    return;
  }
  if (response.status_code == net::HTTP_CONFLICT) {
    callback(ledger::Result::ALREADY_EXISTS);
    return;
  }

  callback(ledger::Result::LEDGER_ERROR);
}

void Wallet::TransferAnonToExternalWallet(
    ledger::ExternalWalletPtr wallet,
    const bool allow_zero_balance,
    ledger::ResultCallback callback) {
  FetchBalance(std::bind(&Wallet::OnTransferAnonToExternalWalletBalance,
               this,
               _1,
               _2,
               *wallet,
               allow_zero_balance,
               callback));
}

void Wallet::OnTransferAnonToExternalWalletBalance(
    ledger::Result result,
    ledger::BalancePtr properties,
    const ledger::ExternalWallet& wallet,
    const bool allow_zero_balance,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK || !properties) {
    BLOG(0, "Anon funds transfer failed");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  if (!allow_zero_balance && properties->user_funds == "0") {
    BLOG(0, "Zero balance not allowed");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  const std::string anon_address =
      ledger_->GetStringState(ledger::kStateUpholdAnonAddress);

  if (!anon_address.empty()) {
    BLOG(1, "Anon address already exists");
    OnTransferAnonToExternalWalletAddress(
        ledger::Result::ALREADY_EXISTS,
        anon_address,
        wallet.address,
        properties->user_funds,
        callback);
    return;
  }

  auto anon_callback =
      std::bind(&Wallet::OnTransferAnonToExternalWalletAddress,
          this,
          _1,
          _2,
          wallet.address,
          properties->user_funds,
          callback);

  auto wallet_ptr = ledger::ExternalWallet::New(wallet);
  uphold_->CreateAnonAddressIfNecessary(std::move(wallet_ptr), anon_callback);
}

std::string Wallet::GetClaimPayload(
    const std::string user_funds,
    const std::string new_address,
    const std::string anon_address) {
  ledger::UnsignedTxProperties unsigned_tx;
  unsigned_tx.amount = user_funds;
  unsigned_tx.currency = "BAT";
  unsigned_tx.destination = new_address;
  const ledger::UnsignedTxState unsigned_tx_state;
  const std::string octets = unsigned_tx_state.ToJson(unsigned_tx);

  std::string header_digest = "SHA-256=" +
      braveledger_bat_helper::getBase64(
          braveledger_bat_helper::getSHA256(octets));

  std::vector<std::string> header_keys;
  header_keys.push_back("digest");
  std::vector<std::string> header_values;
  header_values.push_back(header_digest);

  const auto seed = braveledger_state::GetRecoverySeed(ledger_);
  std::vector<uint8_t> secret_key = braveledger_bat_helper::getHKDF(seed);
  std::vector<uint8_t> public_key;
  std::vector<uint8_t> new_secret_key;
  bool success = braveledger_bat_helper::getPublicKeyFromSeed(
      secret_key,
      &public_key,
      &new_secret_key);
  if (!success) {
    return "";
  }

  std::string header_signature = braveledger_bat_helper::sign(
      header_keys,
      header_values,
      "primary",
      new_secret_key);

  base::Value payload(base::Value::Type::DICTIONARY);

  base::Value signed_tx(base::Value::Type::DICTIONARY);
  signed_tx.SetStringKey("octets", octets);

  base::Value denomination(base::Value::Type::DICTIONARY);
  denomination.SetStringKey("amount", user_funds);
  denomination.SetStringKey("currency", "BAT");

  base::Value body(base::Value::Type::DICTIONARY);
  body.SetStringKey("destination", new_address);
  body.SetKey("denomination", std::move(denomination));
  signed_tx.SetKey("body", std::move(body));

  base::Value headers(base::Value::Type::DICTIONARY);
  headers.SetStringKey("digest", header_digest);
  headers.SetStringKey("signature", header_signature);
  signed_tx.SetKey("headers", std::move(headers));

  payload.SetKey("signedTx", std::move(signed_tx));
  payload.SetStringKey("anonymousAddress", anon_address);

  std::string json;
  base::JSONWriter::Write(payload, &json);

  return json;
}

void Wallet::OnTransferAnonToExternalWalletAddress(
    ledger::Result result,
    const std::string& anon_address,
    const std::string& new_address,
    const std::string& user_funds,
    ledger::ResultCallback callback) {
  if ((result != ledger::Result::LEDGER_OK &&
      result != ledger::Result::ALREADY_EXISTS)
      || anon_address.empty()) {
    BLOG(0, "Anon funds transfer failed");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  if (result != ledger::Result::ALREADY_EXISTS) {
    ledger_->SetStringState(ledger::kStateUpholdAnonAddress, anon_address);
  }

  const std::string path = base::StringPrintf(
      "%s%s/claim",
      WALLET_PROPERTIES,
      ledger_->GetPaymentId().c_str());

  const std::string url = braveledger_request_util::BuildUrl(
      path,
      PREFIX_V2);

  auto transfer_callback = std::bind(&Wallet::OnTransferAnonToExternalWallet,
                          this,
                          _1,
                          callback);

  const std::string payload = GetClaimPayload(
      user_funds,
      new_address,
      anon_address);

  if (payload.empty()) {
    BLOG(0, "Payload is empty");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  std::vector<std::string> wallet_header;
  wallet_header.push_back("Content-Type: application/json; charset=UTF-8");

  ledger_->LoadURL(
      url,
      wallet_header,
      payload,
      "application/json; charset=utf-8",
      ledger::UrlMethod::POST,
      transfer_callback);
}

void Wallet::GetAnonWalletStatus(ledger::ResultCallback callback) {
  const std::string payment_id = ledger_->GetPaymentId();
  const std::string passphrase = GetWalletPassphrase();
  const uint64_t stamp = ledger_->GetCreationStamp();

  if (!payment_id.empty() && stamp != 0) {
    callback(ledger::Result::WALLET_CREATED);
    return;
  }

  if (payment_id.empty() || passphrase.empty()) {
    BLOG(0, "Wallet is corrupted");
    callback(ledger::Result::CORRUPTED_DATA);
    return;
  }

  callback(ledger::Result::LEDGER_OK);
}

}  // namespace braveledger_wallet
