/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "chrome/browser/lifetime/application_lifetime.h"

#include "base/strings/stringprintf.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/endpoint/uphold/uphold_server.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/notifications/notification_keys.h"
#include "bat/ledger/internal/uphold/uphold_transfer.h"
#include "bat/ledger/internal/uphold/uphold_util.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace ledger::uphold {

UpholdTransfer::UpholdTransfer(LedgerImpl* ledger)
    : ledger_((DCHECK(ledger), ledger)),
      uphold_server_(std::make_unique<endpoint::UpholdServer>(ledger)) {}

UpholdTransfer::~UpholdTransfer() = default;

void UpholdTransfer::CreateTransaction(
    const Transaction& transaction,
    client::CreateTransactionCallback callback) {
  const auto wallet = ledger_->uphold()->GetWallet();
  if (!wallet) {
    BLOG(0, "Uphold wallet is null!");
    return std::move(callback).Run(type::Result::LEDGER_ERROR, "");
  }

  if (wallet->status != type::WalletStatus::VERIFIED) {
    BLOG(0, "Uphold wallet status should have been VERIFIED!");
    return std::move(callback).Run(type::Result::LEDGER_ERROR, "");
  }

  CheckWalletState(wallet.get());

  uphold_server_->post_transaction()->Request(
      wallet->token, wallet->address, transaction,
      base::BindOnce(&UpholdTransfer::OnCreateTransaction,
                     base::Unretained(this), std::move(callback)));
}

void UpholdTransfer::OnCreateTransaction(
    client::CreateTransactionCallback callback,
    type::Result result,
    std::string&& transaction_id) {
  const auto wallet = ledger_->uphold()->GetWallet();
  if (!wallet) {
    BLOG(0, "Uphold wallet is null!");
    return std::move(callback).Run(type::Result::LEDGER_ERROR, "");
  }

  if (wallet->status != type::WalletStatus::VERIFIED) {
    BLOG(0, "Uphold wallet status should have been VERIFIED!");
    return std::move(callback).Run(type::Result::LEDGER_ERROR, "");
  }

  CheckWalletState(wallet.get());

  if (result == type::Result::EXPIRED_TOKEN) {
    ledger_->uphold()->DisconnectWallet(
        ledger::notifications::kWalletDisconnected);
    return std::move(callback).Run(type::Result::EXPIRED_TOKEN, "");
  }

  if (result != type::Result::LEDGER_OK || transaction_id.empty()) {
    // TODO(nejczdovc): add retry logic to all errors in this function
    return std::move(callback).Run(type::Result::LEDGER_ERROR, "");
  }

  std::move(callback).Run(result, std::move(transaction_id));
}

void UpholdTransfer::CommitTransaction(const std::string& transaction_id,
                                       ledger::ResultCallback callback) {
  const auto wallet = ledger_->uphold()->GetWallet();
  if (!wallet) {
    BLOG(0, "Uphold wallet is null!");
    return std::move(callback).Run(type::Result::LEDGER_ERROR);
  }

  if (wallet->status != type::WalletStatus::VERIFIED) {
    BLOG(0, "Uphold wallet status should have been VERIFIED!");
    return std::move(callback).Run(type::Result::LEDGER_ERROR);
  }

  CheckWalletState(wallet.get());

  if (transaction_id.empty()) {
    BLOG(0, "Transaction ID is empty!");
    return std::move(callback).Run(type::Result::LEDGER_ERROR);
  }

  uphold_server_->post_transaction_commit()->Request(
      wallet->token, wallet->address, transaction_id,
      base::BindOnce(&UpholdTransfer::OnCommitTransaction,
                     base::Unretained(this), std::move(callback)));
}

void UpholdTransfer::OnCommitTransaction(ledger::ResultCallback callback,
                                         type::Result result) {
  // chrome::AttemptUserExit();
  const auto wallet = ledger_->uphold()->GetWallet();
  if (!wallet) {
    BLOG(0, "Uphold wallet is null!");
    return std::move(callback).Run(type::Result::LEDGER_ERROR);
  }

  if (wallet->status != type::WalletStatus::VERIFIED) {
    BLOG(0, "Uphold wallet status should have been VERIFIED!");
    return std::move(callback).Run(type::Result::LEDGER_ERROR);
  }

  CheckWalletState(wallet.get());

  if (result == type::Result::EXPIRED_TOKEN) {
    ledger_->uphold()->DisconnectWallet(
        ledger::notifications::kWalletDisconnected);
    return std::move(callback).Run(type::Result::EXPIRED_TOKEN);
  }

  if (result != type::Result::LEDGER_OK) {
    return std::move(callback).Run(type::Result::LEDGER_ERROR);
  }

  std::move(callback).Run(result);
}

}  // namespace ledger::uphold
