/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet/wallet_create.h"

#include <vector>

#include "base/json/json_reader.h"
#include "bat/ledger/internal/common/security_util.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/ledger_impl.h"

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
  const auto payment_id = ledger_->state()->GetPaymentId();

  if (!payment_id.empty()) {
    BLOG(1, "Wallet already exists");
    callback(type::Result::WALLET_CREATED);
    return;
  }

  const auto key_info_seed = util::Security::GenerateSeed();
  ledger_->state()->SetRecoverySeed(key_info_seed);

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

  ledger_->state()->SetPaymentId(payment_id);

  ledger_->state()->SetRewardsMainEnabled(true);
  ledger_->state()->SetAutoContributeEnabled(true);
  ledger_->state()->ResetReconcileStamp();
  if (!ledger::is_testing) {
    ledger_->state()->SetFetchOldBalanceEnabled(false);
    ledger_->state()->SetEmptyBalanceChecked(true);
    ledger_->state()->SetPromotionCorruptedMigrated(true);
  }
  ledger_->state()->SetCreationStamp(
      util::GetCurrentTimeStamp());
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
