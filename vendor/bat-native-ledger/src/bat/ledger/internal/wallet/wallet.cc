/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet/wallet.h"

#include <utility>

#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/rapidjson_bat_helper.h"
#include "bat/ledger/internal/state_keys.h"
#include "bat/ledger/internal/wallet/balance.h"
#include "bat/ledger/internal/wallet/create.h"
#include "bat/ledger/internal/wallet/recover.h"
#include "bat/ledger/internal/wallet/wallet_util.h"
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

void Wallet::CreateWalletIfNecessary(const std::string& safetynet_token,
    ledger::CreateWalletCallback callback) {
  const auto payment_id = ledger_->GetPaymentId();
  const auto stamp = ledger_->GetBootStamp();
  const auto persona_id = ledger_->GetPersonaId();

  if (!payment_id.empty() && stamp != 0 && !persona_id.empty()) {
    callback(ledger::Result::WALLET_CREATED);
    return;
  }

  BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
     "Wallet creation didn't finish or corrupted. " <<
     "We need to clear persona Id and start again";
  ledger_->SetPersonaId("");

  create_->Start(safetynet_token, std::move(callback));
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
    ledger::DisconnectWalletCallback callback,
    std::map<std::string, ledger::ExternalWalletPtr> wallets) {
  if (wallets.size() == 0) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto wallet_ptr = GetWallet(wallet_type, std::move(wallets));

  if (!wallet_ptr) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  wallet_ptr = ResetWallet(std::move(wallet_ptr));

  ledger_->SaveExternalWallet(wallet_type, std::move(wallet_ptr));
  callback(ledger::Result::LEDGER_OK);
}

void Wallet::DisconnectWallet(
      const std::string& wallet_type,
      ledger::DisconnectWalletCallback callback) {
  auto wallet_callback = std::bind(&Wallet::OnDisconnectWallet,
                                   this,
                                   wallet_type,
                                   callback,
                                   _1);

  ledger_->GetExternalWallets(wallet_callback);
}

void Wallet::OnTransferAnonToExternalWallet(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    ledger::TransferAnonToExternalWalletCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code == net::HTTP_OK ||
      response_status_code == net::HTTP_CONFLICT) {
    callback(ledger::Result::LEDGER_OK);
    return;
  }

  callback(ledger::Result::LEDGER_ERROR);
}

void Wallet::TransferAnonToExternalWallet(
    ledger::ExternalWalletPtr wallet,
    const bool allow_zero_balance,
    ledger::TransferAnonToExternalWalletCallback callback) {

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
    ledger::TransferAnonToExternalWalletCallback callback) {
  if (result != ledger::Result::LEDGER_OK || !properties) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  if (!allow_zero_balance && properties->user_funds == "0") {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  const std::string anon_address =
      ledger_->GetStringState(ledger::kStateUpholdAnonAddress);

  if (!anon_address.empty()) {
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
  braveledger_bat_helper::WALLET_INFO_ST wallet_info = ledger_->GetWalletInfo();

  braveledger_bat_helper::UNSIGNED_TX unsigned_tx;
  unsigned_tx.amount_ = user_funds;
  unsigned_tx.currency_ = "BAT";
  unsigned_tx.destination_ = new_address;
  const std::string octets =
      braveledger_bat_helper::stringifyUnsignedTx(unsigned_tx);

  std::string header_digest = "SHA-256=" +
      braveledger_bat_helper::getBase64(
          braveledger_bat_helper::getSHA256(octets));

  std::string header_keys[1] = {"digest"};
  std::string header_values[1] = {header_digest};

  std::vector<uint8_t> secret_key = braveledger_bat_helper::getHKDF(
      wallet_info.keyInfoSeed_);
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
      1,
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
    ledger::TransferAnonToExternalWalletCallback callback) {
  if ((result != ledger::Result::LEDGER_OK &&
      result != ledger::Result::ALREADY_EXISTS)
      || anon_address.empty()) {
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

  const std::string url = braveledger_bat_helper::buildURL(
      path,
      PREFIX_V2,
      braveledger_bat_helper::SERVER_TYPES::LEDGER);

  auto transfer_callback = std::bind(&Wallet::OnTransferAnonToExternalWallet,
                          this,
                          _1,
                          _2,
                          _3,
                          callback);

  const std::string payload = GetClaimPayload(
      user_funds,
      new_address,
      anon_address);

  if (payload.empty()) {
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
      ledger::URL_METHOD::POST,
      transfer_callback);
}

void Wallet::GetGrantViaSafetynetCheck(const std::string& promotion_id) {
  ledger_->LoadURL(braveledger_bat_helper::buildURL(
      (std::string)GET_PROMOTION_ATTESTATION + ledger_->GetPaymentId(),
      PREFIX_V1),
  std::vector<std::string>(), "", "",
  ledger::URL_METHOD::GET, std::bind(&Wallet::GetGrantViaSafetynetCheckCallback,
                                   this,
                                   promotion_id,
                                   _1,
                                   _2,
                                   _3));
}

void Wallet::GetGrantViaSafetynetCheckCallback(const std::string& promotion_id,
                                                  int response_status_code,
                                                  const std::string& response,
                                                  const std::map<std::string,
                                                  std::string>& headers) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code != net::HTTP_OK) {
    // Attestation failed
    braveledger_bat_helper::GRANT grant;
    grant.promotionId = promotion_id;
    ledger_->OnGrantFinish(ledger::Result::SAFETYNET_ATTESTATION_FAILED,
        grant);
    return;
  }
  std::string nonce;
  braveledger_bat_helper::getJSONValue("nonce", response, &nonce);
  ledger_->OnGrantViaSafetynetCheck(promotion_id, nonce);
}

}  // namespace braveledger_wallet
