/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/wallet/recover.h"

#include<utility>

#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/legacy/wallet_info_properties.h"
#include "bat/ledger/internal/request/request_util.h"
#include "bat/ledger/internal/state/state_util.h"
#include "net/http/http_status_code.h"

#include "anon/anon.h"
#include "wally_bip39.h"  // NOLINT

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace {

ledger::Result ParseRecoverKeyResponse(
    const std::string& response,
    std::string* payment_id) {
  DCHECK(payment_id);

  base::Optional<base::Value> value = base::JSONReader::Read(response);
  if (!value || !value->is_dict()) {
    return ledger::Result::LEDGER_ERROR;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return ledger::Result::LEDGER_ERROR;
  }

  const auto* payment_id_string = dictionary->FindStringKey("paymentId");
  if (!payment_id_string || payment_id_string->empty()) {
    BLOG(0, "Payment id is wrong");
    return ledger::Result::LEDGER_ERROR;
  }

  *payment_id = *payment_id_string;
  return ledger::Result::LEDGER_OK;
}

ledger::Result ParseRecoverWalletResponse(
    const std::string& response,
    std::string* card_id,
    double* balance) {
  DCHECK(card_id && balance);

  base::Optional<base::Value> value = base::JSONReader::Read(response);
  if (!value || !value->is_dict()) {
    return ledger::Result::LEDGER_ERROR;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return ledger::Result::LEDGER_ERROR;
  }

  const auto* balance_string = dictionary->FindStringKey("balance");
  if (!balance_string) {
    BLOG(0, "Balance is wrong");
    return ledger::Result::LEDGER_ERROR;
  }

  const auto* card_id_string = dictionary->FindStringPath("addresses.CARD_ID");
  if (!card_id_string || card_id_string->empty()) {
    BLOG(0, "Card id is wrong");
    return ledger::Result::LEDGER_ERROR;
  }

  const bool success = base::StringToDouble(*balance_string, balance);
  if (!success) {
    *balance = 0.0;
  }

  *card_id = *card_id_string;
  return ledger::Result::LEDGER_OK;
}

}  // namespace

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
  if (response.status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR, 0);
    return;
  }

  std::string recovery_id;
  const auto result = ParseRecoverKeyResponse(response.body, &recovery_id);
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
  if (response.status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR, 0);
    return;
  }

  double balance = .0;
  std::string card_id;
  const auto result = ParseRecoverWalletResponse(
      response.body,
      &card_id,
      &balance);
  if (result == ledger::Result::LEDGER_ERROR) {
    callback(result, 0.0);
    return;
  }

  braveledger_state::SetRecoverySeed(ledger_, new_seed);
  braveledger_state::SetPaymentId(ledger_, recovery_id);
  braveledger_state::SetAnonymousCardId(ledger_, card_id);
  braveledger_state::SetFetchOldBalanceEnabled(ledger_, true);
  ledger_->SetConfirmationsWalletInfo();

  callback(ledger::Result::LEDGER_OK, balance);
}

}  // namespace braveledger_wallet
