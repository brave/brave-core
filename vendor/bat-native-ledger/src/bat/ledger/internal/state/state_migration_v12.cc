/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/state/state_migration_v12.h"

#include <memory>
#include <utility>

#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/uphold/uphold.h"
#include "bat/ledger/internal/uphold/uphold_transfer.h"
#include "bat/ledger/internal/uphold/uphold_util.h"

using ledger::uphold::Transaction;

namespace ledger::state {

StateMigrationV12::StateMigrationV12(LedgerImpl* ledger)
    : transfer_(std::make_unique<uphold::UpholdTransfer>(ledger)),
      ledger_((DCHECK(ledger), ledger)) {}

StateMigrationV12::~StateMigrationV12() = default;

void StateMigrationV12::Migrate(ledger::LegacyResultCallback callback) {
  auto wallet = ledger_->uphold()->GetWallet();
  if (!wallet) {
    BLOG(1, "Wallet is null.");
    return callback(type::Result::LEDGER_OK);
  }

  CreateTransactionForFee(std::move(callback), std::move(wallet));
}

void StateMigrationV12::CreateTransactionForFee(
    ledger::LegacyResultCallback callback,
    type::ExternalWalletPtr wallet) {
  DCHECK(wallet);

  if (!wallet->fees.empty()) {
    const auto& [contribution_id, amount] = *wallet->fees.begin();

    Transaction transaction{uphold::GetFeeAddress(), amount,
                            /*uphold::kFeeMessage*/ "TODO"};

    transfer_->CreateTransaction(
        std::move(transaction),
        base::BindOnce(&StateMigrationV12::OnCreateTransactionForFee,
                       base::Unretained(this), std::move(callback),
                       contribution_id));
  } else {
    callback(type::Result::LEDGER_OK);
  }
}

void StateMigrationV12::OnCreateTransactionForFee(
    ledger::LegacyResultCallback callback,
    const std::string& contribution_id,
    type::Result result,
    std::string&& transaction_id) {
  DCHECK(!contribution_id.empty());

  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Failed to create transaction with Uphold!");
    return callback(type::Result::LEDGER_ERROR);
  }

  DCHECK(!transaction_id.empty());

  auto uphold_wallet = ledger_->uphold()->GetWallet();
  if (!uphold_wallet) {
    BLOG(0, "Uphold wallet is null!");
    return callback(type::Result::LEDGER_ERROR);
  }

  if (!uphold_wallet->fees.erase(contribution_id)) {
    BLOG(0, "Failed to remove fee!");
    return callback(type::Result::LEDGER_ERROR);
  }

  if (!ledger_->uphold()->SetWallet(std::move(uphold_wallet))) {
    BLOG(0, "Failed to set Uphold wallet!");
    return callback(type::Result::LEDGER_ERROR);
  }

  auto external_transaction = type::ExternalTransaction::New(
      type::WalletProvider::UPHOLD, std::move(transaction_id), contribution_id,
      true, type::ExternalTransactionStatus::STATUS_0);

  ledger_->database()->SaveExternalTransaction(
      std::move(external_transaction),
      base::BindOnce(&StateMigrationV12::OnSaveExternalTransactionForFee,
                     base::Unretained(this), std::move(callback)));
}

void StateMigrationV12::OnSaveExternalTransactionForFee(
    ledger::LegacyResultCallback callback,
    type::Result result) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Failed to save external transaction for fee!");
    return callback(type::Result::LEDGER_ERROR);
  }

  auto uphold_wallet = ledger_->uphold()->GetWallet();
  if (!uphold_wallet) {
    BLOG(0, "Uphold wallet is null!");
    return callback(type::Result::LEDGER_ERROR);
  }

  CreateTransactionForFee(std::move(callback), std::move(uphold_wallet));
}

}  // namespace ledger::state
