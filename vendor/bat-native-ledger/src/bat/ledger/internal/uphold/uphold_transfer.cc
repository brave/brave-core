/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/endpoint/uphold/uphold_server.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/notifications/notification_keys.h"
#include "bat/ledger/internal/uphold/uphold_transfer.h"
#include "bat/ledger/internal/uphold/uphold_util.h"
#include "bat/ledger/internal/wallet/wallet_util.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace ledger {
namespace uphold {

UpholdTransfer::UpholdTransfer(LedgerImpl* ledger)
    : ledger_(ledger),
      uphold_server_(std::make_unique<endpoint::UpholdServer>(ledger)) {}

UpholdTransfer::~UpholdTransfer() = default;

void UpholdTransfer::Start(const Transaction& transaction,
                           client::TransactionCallback callback) {
  auto uphold_wallet = ledger_->uphold()->GetWallet();
  if (!uphold_wallet) {
    BLOG(0, "Uphold wallet is null!");
    return callback(type::Result::LEDGER_ERROR, "");
  }

  if (uphold_wallet->status != type::WalletStatus::VERIFIED) {
    BLOG(0, "Wallet status should have been VERIFIED!");
    return callback(type::Result::LEDGER_ERROR, "");
  }

  DCHECK(!uphold_wallet->token.empty());
  DCHECK(!uphold_wallet->address.empty());

  auto url_callback =
      std::bind(&UpholdTransfer::OnCreateTransaction, this, _1, _2, callback);

  uphold_server_->post_transaction()->Request(
      uphold_wallet->token, uphold_wallet->address, transaction, url_callback);
}

void UpholdTransfer::OnCreateTransaction(const type::Result result,
                                         const std::string& id,
                                         client::TransactionCallback callback) {
  auto uphold_wallet = ledger_->uphold()->GetWallet();
  if (!uphold_wallet) {
    BLOG(0, "Uphold wallet is null!");
    return callback(type::Result::LEDGER_ERROR, "");
  }

  if (uphold_wallet->status != type::WalletStatus::VERIFIED) {
    BLOG(0, "Wallet status should have been VERIFIED!");
    return callback(type::Result::LEDGER_ERROR, "");
  }

  DCHECK(!uphold_wallet->token.empty());
  DCHECK(!uphold_wallet->address.empty());

  if (result == type::Result::EXPIRED_TOKEN) {
    ledger_->uphold()->DisconnectWallet(
        ledger::notifications::kWalletDisconnected);
    return callback(type::Result::EXPIRED_TOKEN, "");
  }

  if (result != type::Result::LEDGER_OK) {
    // TODO(nejczdovc): add retry logic to all errors in this function
    return callback(type::Result::LEDGER_ERROR, "");
  }

  CommitTransaction(id, callback);
}

void UpholdTransfer::CommitTransaction(const std::string& transaction_id,
                                       client::TransactionCallback callback) {
  auto uphold_wallet = ledger_->uphold()->GetWallet();
  if (!uphold_wallet) {
    BLOG(0, "Uphold wallet is null!");
    return callback(type::Result::LEDGER_ERROR, "");
  }

  if (transaction_id.empty()) {
    BLOG(0, "Transaction id not found");
    return callback(type::Result::LEDGER_ERROR, "");
  }

  auto url_callback = std::bind(&UpholdTransfer::OnCommitTransaction, this, _1,
                                transaction_id, callback);

  uphold_server_->post_transaction_commit()->Request(
      uphold_wallet->token, uphold_wallet->address, transaction_id,
      url_callback);
}

void UpholdTransfer::OnCommitTransaction(const type::Result result,
                                         const std::string& transaction_id,
                                         client::TransactionCallback callback) {
  auto uphold_wallet = ledger_->uphold()->GetWallet();
  if (!uphold_wallet) {
    BLOG(0, "Uphold wallet is null!");
    return callback(type::Result::LEDGER_ERROR, "");
  }

  if (uphold_wallet->status != type::WalletStatus::VERIFIED) {
    BLOG(0, "Wallet status should have been VERIFIED!");
    return callback(type::Result::LEDGER_ERROR, "");
  }

  DCHECK(!uphold_wallet->token.empty());
  DCHECK(!uphold_wallet->address.empty());

  if (result == type::Result::EXPIRED_TOKEN) {
    ledger_->uphold()->DisconnectWallet(
        ledger::notifications::kWalletDisconnected);
    return callback(type::Result::EXPIRED_TOKEN, "");
  }

  if (result != type::Result::LEDGER_OK) {
    return callback(type::Result::LEDGER_ERROR, "");
  }

  callback(type::Result::LEDGER_OK, transaction_id);
}

}  // namespace uphold
}  // namespace ledger
