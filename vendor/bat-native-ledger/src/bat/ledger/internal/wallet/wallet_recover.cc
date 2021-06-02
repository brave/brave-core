/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet/wallet_recover.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/string_split.h"
#include "bat/ledger/internal/common/security_util.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/logging/event_log_keys.h"

#include "wally_bip39.h"  // NOLINT

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace ledger {
namespace wallet {

WalletRecover::WalletRecover(LedgerImpl* ledger) :
    ledger_(ledger),
    promotion_server_(std::make_unique<endpoint::PromotionServer>(ledger)) {
  DCHECK(ledger_);
}

WalletRecover::~WalletRecover() = default;

void WalletRecover::Start(
    const std::string& pass_phrase,
    ledger::ResultCallback callback) {
  if (pass_phrase.empty()) {
    BLOG(0, "Pass phrase is empty");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  auto phrase_split = base::SplitString(
      pass_phrase,
      " ",
      base::TRIM_WHITESPACE,
      base::SPLIT_WANT_NONEMPTY);

  if (phrase_split.size() == 16) {
    BLOG(0, "Pass phrase is niceware, update to bip39");
    callback(type::Result::CORRUPTED_DATA);
    return;
  }

  std::vector<unsigned char> new_seed;
  new_seed.resize(32);
  size_t written = 0;
  const int result = bip39_mnemonic_to_bytes(
      nullptr,
      pass_phrase.c_str(),
      &new_seed.front(),
      new_seed.size(),
      &written);

  if (result != 0 || written == 0) {
    BLOG(0, "Result: " << result << " Size: " << written);
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  const std::string public_key_hex =
      util::Security::GetPublicKeyHexFromSeed(new_seed);

  auto url_callback = std::bind(&WalletRecover::OnRecover,
      this,
      _1,
      _2,
      _3,
      new_seed,
      callback);

  promotion_server_->get_recover_wallet()->Request(
      public_key_hex,
      url_callback);
}

void WalletRecover::OnRecover(
    const type::Result result,
    const std::string& payment_id,
    const bool legacy_wallet,
    const std::vector<uint8_t>& new_seed,
    ledger::ResultCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    callback(result);
    return;
  }

  auto wallet = type::BraveWallet::New();
  wallet->payment_id = payment_id;
  wallet->recovery_seed = new_seed;
  const bool success = ledger_->wallet()->SetWallet(std::move(wallet));

  if (!success) {
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  ledger_->state()->SetPromotionLastFetchStamp(0);
  ledger_->state()->SetPromotionCorruptedMigrated(true);
  if (legacy_wallet) {
    ledger_->state()->SetFetchOldBalanceEnabled(true);
  }
  ledger_->state()->SetCreationStamp(util::GetCurrentTimeStamp());
  ledger_->database()->SaveEventLog(log::kWalletRecovered, payment_id);
  callback(type::Result::LEDGER_OK);
}

}  // namespace wallet
}  // namespace ledger
