/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet/wallet_create.h"

#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "bat/ledger/internal/common/security_util.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/constants.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace ledger {
namespace wallet {

WalletCreate::WalletCreate(LedgerImpl* ledger) :
    ledger_(ledger),
    promotion_server_(std::make_unique<endpoint::PromotionServer>(ledger)) {
  DCHECK(ledger_);
}

WalletCreate::~WalletCreate() = default;

void WalletCreate::Start(ledger::ResultCallback callback) {
  auto wallet = ledger_->wallet()->GetWallet();

  if (wallet && !wallet->payment_id.empty()) {
    BLOG(1, "Wallet already exists");
    callback(type::Result::WALLET_CREATED);
    return;
  }

  wallet = type::BraveWallet::New();
  const auto key_info_seed = util::Security::GenerateSeed();
  wallet->recovery_seed = key_info_seed;
  ledger_->wallet()->SetWallet(std::move(wallet));

  auto url_callback = std::bind(&WalletCreate::OnCreate,
      this,
      _1,
      _2,
      callback);

  promotion_server_->post_wallet_brave()->Request(url_callback);
}

void WalletCreate::OnCreate(
    const type::Result result,
    const std::string& payment_id,
    ledger::ResultCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    callback(result);
    return;
  }

  auto wallet = ledger_->wallet()->GetWallet();
  wallet->payment_id = payment_id;
  const bool success = ledger_->wallet()->SetWallet(std::move(wallet));

  if (!success) {
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  ledger_->publisher()->CalcScoreConsts(
      ledger_->state()->GetPublisherMinVisitTime());
  ledger_->state()->ResetReconcileStamp();
  if (!ledger::is_testing) {
    ledger_->state()->SetFetchOldBalanceEnabled(false);
    ledger_->state()->SetEmptyBalanceChecked(true);
    ledger_->state()->SetPromotionCorruptedMigrated(true);
  }
  ledger_->state()->SetCreationStamp(util::GetCurrentTimeStamp());
  ledger_->state()->SetInlineTippingPlatformEnabled(
      type::InlineTipsPlatforms::REDDIT,
      true);
  ledger_->state()->SetInlineTippingPlatformEnabled(
      type::InlineTipsPlatforms::TWITTER,
      true);
  ledger_->state()->SetInlineTippingPlatformEnabled(
      type::InlineTipsPlatforms::GITHUB,
      true);
  callback(type::Result::WALLET_CREATED);
}

}  // namespace wallet
}  // namespace ledger
