/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet/recover.h"

#include <utility>

#include "base/strings/string_split.h"
#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/legacy/wallet_info_properties.h"
#include "bat/ledger/internal/request/request_util.h"
#include "bat/ledger/internal/response/response_wallet.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/internal/state/state_util.h"

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
  if (pass_phrase.empty()) {
    BLOG(0, "Pass phrase is empty");
    callback(ledger::Result::LEDGER_ERROR, 0);
    return;
  }

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

  BLOG(0, "Failed to load niceware list");
  callback(result, 0);
  return;
}

void Recover::ContinueRecover(
    int result,
    size_t* written,
    const std::vector<uint8_t>& newSeed,
    ledger::RecoverWalletCallback callback) {
  if (result != 0 || *written == 0) {
    BLOG(1, "Result: " << result << " Size: " << *written);
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
                            newSeed,
                            std::move(callback));
  const auto url = braveledger_request_util::BuildUrl(
        (std::string)RECOVER_WALLET_PUBLIC_KEY + publicKeyHex,
        PREFIX_V2);
  ledger_->LoadURL(url, {}, "", "", ledger::UrlMethod::GET, std::move(on_load));
}

void Recover::RecoverWalletPublicKeyCallback(
    const ledger::UrlResponse& response,
    const std::vector<uint8_t>& new_seed,
    ledger::RecoverWalletCallback callback) {
  BLOG(6, ledger::UrlResponseToString(__func__, response));

  std::string recovery_id;
  const auto result =
      braveledger_response_util::ParseWalletRecoverKey(response, &recovery_id);
  if (result == ledger::Result::LEDGER_ERROR) {
    callback(result, 0.0);
    return;
  }

  auto recover_callback = std::bind(&Recover::RecoverWalletCallback,
      this,
      _1,
      recovery_id,
      new_seed,
      std::move(callback));
  const std::string url = braveledger_request_util::BuildUrl
      ("/wallet/" + recovery_id, PREFIX_V2);
  ledger_->LoadURL(url, {}, "", "", ledger::UrlMethod::GET, recover_callback);
}

void Recover::RecoverWalletCallback(
    const ledger::UrlResponse& response,
    const std::string& recovery_id,
    const std::vector<uint8_t>& new_seed,
    ledger::RecoverWalletCallback callback) {
  BLOG(6, ledger::UrlResponseToString(__func__, response));

  double balance = 0.0;
  std::string card_id;
  const auto result = braveledger_response_util::ParseRecoverWallet(
      response,
      &card_id,
      &balance);
  if (result != ledger::Result::LEDGER_OK) {
    callback(result, balance);
    return;
  }

  braveledger_state::SetRecoverySeed(ledger_, new_seed);
  braveledger_state::SetPaymentId(ledger_, recovery_id);
  braveledger_state::SetAnonymousCardId(ledger_, card_id);
  braveledger_state::SetFetchOldBalanceEnabled(ledger_, true);
  ledger_->SetUint64State(ledger::kStatePromotionLastFetchStamp, 0);
  ledger_->SetConfirmationsWalletInfo();

  callback(ledger::Result::LEDGER_OK, balance);
}

}  // namespace braveledger_wallet
