/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet/wallet_create.h"

#include <vector>

#include "base/json/json_reader.h"
#include "bat/ledger/internal/common/security_helper.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/request/request_promotion.h"
#include "bat/ledger/internal/request/request_util.h"
#include "bat/ledger/internal/response/response_wallet.h"

using std::placeholders::_1;

namespace braveledger_wallet {

WalletCreate::WalletCreate(bat_ledger::LedgerImpl* ledger) : ledger_(ledger) {
}

WalletCreate::~WalletCreate() = default;

void WalletCreate::Start(ledger::ResultCallback callback) {
  const auto payment_id = ledger_->state()->GetPaymentId();
  const auto stamp = ledger_->state()->GetCreationStamp();

  if (!payment_id.empty() && stamp != 0) {
    BLOG(1, "Wallet already exists");
    callback(ledger::Result::WALLET_CREATED);
    return;
  }

  auto key_info_seed = braveledger_helper::Security::GenerateSeed();
  ledger_->state()->SetRecoverySeed(key_info_seed);

  const std::string public_key_hex =
      braveledger_helper::Security::GetPublicKeyHexFromSeed(key_info_seed);

  auto url_callback = std::bind(&WalletCreate::OnCreate,
      this,
      _1,
      callback);

  const auto headers = braveledger_request_util::BuildSignHeaders(
      "post /v3/wallet/brave",
      "",
      public_key_hex,
      ledger_->state()->GetRecoverySeed());

  const std::string url = braveledger_request_util::GetCreateWalletURL();
  ledger_->LoadURL(
      url,
      headers,
      "",
      "application/json; charset=utf-8",
      ledger::UrlMethod::POST,
      url_callback);
}

void WalletCreate::OnCreate(
    const ledger::UrlResponse& response,
    ledger::ResultCallback callback) {
  BLOG(6, ledger::UrlResponseToString(__func__, response));

  std::string payment_id;
  const auto result = braveledger_response_util::ParseCreateWallet(
      response,
      &payment_id);

  if (result != ledger::Result::LEDGER_OK) {
    callback(result);
    return;
  }

  ledger_->state()->SetPaymentId(payment_id);

  ledger_->state()->SetRewardsMainEnabled(true);
  ledger_->state()->SetAutoContributeEnabled(true);
  ledger_->state()->ResetReconcileStamp();
  if (!ledger::is_testing) {
    ledger_->state()->SetFetchOldBalanceEnabled(false);
  }
  ledger_->state()->SetCreationStamp(
      braveledger_time_util::GetCurrentTimeStamp());
  ledger_->state()->SetInlineTippingPlatformEnabled(
      ledger::InlineTipsPlatforms::REDDIT,
      true);
  ledger_->state()->SetInlineTippingPlatformEnabled(
      ledger::InlineTipsPlatforms::TWITTER,
      true);
  ledger_->state()->SetInlineTippingPlatformEnabled(
      ledger::InlineTipsPlatforms::GITHUB,
      true);
  callback(ledger::Result::WALLET_CREATED);
}

}  // namespace braveledger_wallet
