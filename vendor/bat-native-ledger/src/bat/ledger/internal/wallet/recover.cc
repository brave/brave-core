/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/wallet/recover.h"

#include<utility>

#include "base/strings/string_split.h"
#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/properties/wallet_info_properties.h"
#include "bat/ledger/internal/request/request_util.h"
#include "net/http/http_status_code.h"

#include "anon/anon.h"
#include "wally_bip39.h"  // NOLINT

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_wallet {

Recover::Recover(bat_ledger::LedgerImpl* ledger) : ledger_(ledger) {
  initAnonize();
}

Recover::~Recover() {
}

void Recover::Start(
    const std::string& pass_phrase,
    ledger::RecoverWalletCallback callback) {
  size_t written = 0;
  auto phrase_split = base::SplitString(
      pass_phrase,
      WALLET_PASSPHRASE_DELIM,
      base::TRIM_WHITESPACE,
      base::SPLIT_WANT_NONEMPTY);

  if (phrase_split.size() == 16) {
    // use niceware for legacy wallet passphrases
    ledger_->LoadNicewareList(std::bind(&Recover::OnNicewareListLoaded,
        this,
        pass_phrase,
        _1,
        _2,
        std::move(callback)));
    return;
  }

  std::vector<unsigned char> newSeed;
  newSeed.resize(32);
  const int result = bip39_mnemonic_to_bytes(
      nullptr,
      pass_phrase.c_str(),
      &newSeed.front(),
      newSeed.size(),
      &written);
  ContinueRecover(result, &written, newSeed, std::move(callback));
}

void Recover::OnNicewareListLoaded(
    const std::string& pass_phrase,
    ledger::Result result,
    const std::string& data,
    ledger::RecoverWalletCallback callback) {
  auto phrase_split = base::SplitString(
      pass_phrase,
      WALLET_PASSPHRASE_DELIM,
      base::TRIM_WHITESPACE,
      base::SPLIT_WANT_NONEMPTY);

  if (result == ledger::Result::LEDGER_OK && phrase_split.size() == 16) {
    std::vector<uint8_t> seed;
    seed.resize(32);
    size_t written = 0;

    auto data_split = base::SplitString(
      data,
      DICTIONARY_DELIMITER,
      base::TRIM_WHITESPACE,
      base::SPLIT_WANT_NONEMPTY);

    uint8_t nwResult = braveledger_bat_helper::niceware_mnemonic_to_bytes(
        pass_phrase,
        &seed,
        &written,
        data_split);
    ContinueRecover(nwResult, &written, seed, std::move(callback));
    return;
  }

  BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
    << "Failed to load niceware list";
  callback(result, 0);
  return;
}

void Recover::ContinueRecover(
    int result,
    size_t* written,
    const std::vector<uint8_t>& newSeed,
    ledger::RecoverWalletCallback callback) {
  if (result != 0 || *written == 0) {
    BLOG(ledger_, ledger::LogLevel::LOG_INFO)
      << "Result: "
      << result
      << " Size: "
      << *written;
    callback(ledger::Result::LEDGER_ERROR, 0);
    return;
  }

  std::vector<uint8_t> secretKey = braveledger_bat_helper::getHKDF(newSeed);
  std::vector<uint8_t> publicKey;
  std::vector<uint8_t> newSecretKey;
  braveledger_bat_helper::getPublicKeyFromSeed(secretKey,
                                               &publicKey,
                                               &newSecretKey);
  std::string publicKeyHex = braveledger_bat_helper::uint8ToHex(publicKey);

  auto on_load = std::bind(&Recover::RecoverWalletPublicKeyCallback,
                            this,
                            _1,
                            _2,
                            _3,
                            newSeed,
                            std::move(callback));
  const auto url = braveledger_request_util::BuildUrl(
        (std::string)RECOVER_WALLET_PUBLIC_KEY + publicKeyHex,
        PREFIX_V2);
  ledger_->LoadURL(
      url,
      std::vector<std::string>(),
      "",
      "",
      ledger::UrlMethod::GET,
      std::move(on_load));
}

void Recover::RecoverWalletPublicKeyCallback(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const std::vector<uint8_t>& new_seed,
    ledger::RecoverWalletCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR, 0);
    return;
  }
  std::string recoveryId;
  braveledger_bat_helper::getJSONValue("paymentId", response, &recoveryId);

  auto on_recover = std::bind(&Recover::RecoverWalletCallback,
                            this,
                            _1,
                            _2,
                            _3,
                            recoveryId,
                            new_seed,
                            std::move(callback));
  ledger_->LoadURL(braveledger_request_util::BuildUrl(
        (std::string)WALLET_PROPERTIES + recoveryId, PREFIX_V2),
      std::vector<std::string>(), "", "", ledger::UrlMethod::GET, on_recover);
}

void Recover::RecoverWalletCallback(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const std::string& recoveryId,
    const std::vector<uint8_t>& new_seed,
    ledger::RecoverWalletCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);
  if (response_status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR, 0);
    return;
  }

  ledger::WalletInfoProperties wallet_info = ledger_->GetWalletInfo();
  ledger::WalletProperties properties = ledger_->GetWalletProperties();
  double fee_amount = .0;
  double balance = .0;
  braveledger_bat_helper::getJSONWalletInfo(
      response,
      &wallet_info,
      &fee_amount);
  braveledger_bat_helper::getJSONRecoverWallet(
      response,
      &balance);
  if (!ledger_->GetUserChangedContribution()) {
    ledger_->SetContributionAmount(fee_amount);
  }
  ledger_->SetWalletProperties(&properties);

  wallet_info.payment_id = recoveryId;
  wallet_info.key_info_seed = new_seed;
  ledger_->SetWalletInfo(wallet_info);
  callback(ledger::Result::LEDGER_OK, balance);
}

}  // namespace braveledger_wallet
