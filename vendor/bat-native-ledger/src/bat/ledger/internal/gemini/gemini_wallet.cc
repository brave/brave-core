/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>

#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/common/random_util.h"
#include "bat/ledger/internal/gemini/gemini_util.h"
#include "bat/ledger/internal/gemini/gemini_wallet.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/logging/event_log_keys.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace ledger {
namespace gemini {

GeminiWallet::GeminiWallet(LedgerImpl* ledger) : ledger_(ledger) {}

GeminiWallet::~GeminiWallet() = default;

void GeminiWallet::Generate(ledger::ResultCallback callback) {
  auto wallet = ledger_->gemini()->GetWallet();
  if (!wallet) {
    wallet = type::ExternalWallet::New();
    wallet->type = constant::kWalletGemini;
    wallet->status = type::WalletStatus::NOT_CONNECTED;
  }

  if (wallet->one_time_string.empty()) {
    wallet->one_time_string = util::GenerateRandomHexString();
  }

  if (wallet->token.empty() &&
      (wallet->status == type::WalletStatus::PENDING)) {
    wallet->status = type::WalletStatus::NOT_CONNECTED;
  }

  wallet = GenerateLinks(std::move(wallet));
  ledger_->gemini()->SetWallet(wallet->Clone());

  if (wallet->status == type::WalletStatus::VERIFIED) {
    ledger_->promotion()->TransferTokens(
        [callback](const type::Result result, const std::string& drain_id) {
          if (result != type::Result::LEDGER_OK) {
            BLOG(0, "Claiming tokens failed");
            callback(type::Result::CONTINUE);
            return;
          }
          callback(type::Result::LEDGER_OK);
        });
    return;
  }

  callback(type::Result::LEDGER_OK);
}

}  // namespace gemini
}  // namespace ledger
