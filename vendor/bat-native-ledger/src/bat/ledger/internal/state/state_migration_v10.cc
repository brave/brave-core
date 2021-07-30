/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/state/state_migration_v10.h"

#include <memory>
#include <utility>

#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/uphold/uphold_util.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace ledger {
namespace state {

StateMigrationV10::StateMigrationV10(LedgerImpl* ledger)
    : ledger_(ledger),
      get_wallet_{
          std::make_unique<ledger::endpoint::promotion::GetWallet>(ledger)} {}

StateMigrationV10::~StateMigrationV10() = default;

void StateMigrationV10::Migrate(ledger::ResultCallback callback) {
  auto uphold_wallet = ledger_->uphold()->GetWallet();
  if (!uphold_wallet) {
    BLOG(1, "Uphold wallet is null.");
    return callback(type::Result::LEDGER_OK);
  }

  switch (uphold_wallet->status) {
    case type::WalletStatus::NOT_CONNECTED:
      uphold_wallet->token = "";
      uphold_wallet->address = "";
      break;
    case type::WalletStatus::CONNECTED:
      uphold_wallet->status = !uphold_wallet->token.empty()
                                  ? type::WalletStatus::PENDING
                                  : type::WalletStatus::NOT_CONNECTED;
      uphold_wallet->address = "";
      break;
    case type::WalletStatus::VERIFIED: {
      if (uphold_wallet->token.empty() || uphold_wallet->address.empty()) {
        uphold_wallet->status = !uphold_wallet->token.empty()
                                    ? type::WalletStatus::PENDING
                                    : type::WalletStatus::DISCONNECTED_VERIFIED;
        uphold_wallet->address = "";
        break;
      }

      auto wallet_info_endpoint_callback = std::bind(
          &StateMigrationV10::OnGetWallet, this, _1, _2, _3, callback);

      if (ledger::is_testing) {
        return wallet_info_endpoint_callback(type::Result::LEDGER_ERROR,
                                             std::string{}, false);
      } else {
        return get_wallet_->Request(std::move(wallet_info_endpoint_callback));
      }
    }
    case type::WalletStatus::DISCONNECTED_NOT_VERIFIED:
      uphold_wallet->status = type::WalletStatus::DISCONNECTED_VERIFIED;
      uphold_wallet->token = "";
      uphold_wallet->address = "";
      break;
    case type::WalletStatus::DISCONNECTED_VERIFIED:
      uphold_wallet->token = "";
      uphold_wallet->address = "";
      break;
    case type::WalletStatus::PENDING:
      uphold_wallet->status = !uphold_wallet->token.empty()
                                  ? type::WalletStatus::PENDING
                                  : type::WalletStatus::NOT_CONNECTED;
      uphold_wallet->address = "";
      break;
    default:
      NOTREACHED();
  }

  uphold_wallet = ledger::uphold::GenerateLinks(std::move(uphold_wallet));
  callback(ledger_->uphold()->SetWallet(std::move(uphold_wallet))
               ? type::Result::LEDGER_OK
               : type::Result::LEDGER_ERROR);
}

void StateMigrationV10::OnGetWallet(type::Result result,
                                    const std::string& custodian,
                                    bool linked,
                                    ledger::ResultCallback callback) {
  auto uphold_wallet = ledger_->uphold()->GetWallet();
  if (!uphold_wallet) {
    BLOG(0, "Uphold wallet is null!");
    return callback(type::Result::LEDGER_ERROR);
  }

  DCHECK(uphold_wallet->status == type::WalletStatus::VERIFIED);
  DCHECK(!uphold_wallet->token.empty());
  DCHECK(!uphold_wallet->address.empty());

  if (result != type::Result::LEDGER_OK ||
      custodian != constant::kWalletUphold ||
      !linked) {  // deemed semi-VERIFIED || semi-VERIFIED
    uphold_wallet->status = type::WalletStatus::PENDING;
    uphold_wallet->address = "";
  }

  uphold_wallet = ledger::uphold::GenerateLinks(std::move(uphold_wallet));
  callback(ledger_->uphold()->SetWallet(std::move(uphold_wallet))
               ? type::Result::LEDGER_OK
               : type::Result::LEDGER_ERROR);
}

}  // namespace state
}  // namespace ledger
