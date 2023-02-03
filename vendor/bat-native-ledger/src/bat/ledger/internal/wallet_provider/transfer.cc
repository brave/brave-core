/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet_provider/transfer.h"

#include <utility>

#include "base/guid.h"
#include "base/strings/string_number_conversions.h"
#include "bat/ledger/internal/ledger_impl.h"

namespace ledger::wallet_provider {

Transfer::Transfer(LedgerImpl* ledger) : ledger_(ledger) {
  DCHECK(ledger_);
}

Transfer::~Transfer() = default;

void Transfer::Run(const std::string& contribution_id,
                   const std::string& destination,
                   double amount,
                   ledger::ResultCallback callback) const {
  MaybeCreateTransaction(
      contribution_id, destination,
      // Rounding to nearest ten-thousandth (0.0001),
      // which supports a minimum tip amount of 0.002 (given a 5% fee).
      base::NumberToString(std::round(amount * 10000) / 10000),
      base::BindOnce(&Transfer::CommitTransaction, base::Unretained(this),
                     std::move(callback)));
}

void Transfer::MaybeCreateTransaction(
    const std::string& contribution_id,
    const std::string& destination,
    const std::string& amount,
    MaybeCreateTransactionCallback callback) const {
  ledger_->database()->GetExternalTransaction(
      contribution_id, destination,
      base::BindOnce(&Transfer::OnGetExternalTransaction,
                     base::Unretained(this), std::move(callback),
                     contribution_id, destination, amount));
}

void Transfer::OnGetExternalTransaction(
    MaybeCreateTransactionCallback callback,
    std::string&& contribution_id,
    std::string&& destination,
    std::string&& amount,
    base::expected<mojom::ExternalTransactionPtr,
                   database::GetExternalTransactionError> existing_transaction)
    const {
  if (existing_transaction.has_value()) {
    DCHECK(existing_transaction.value());
    return std::move(callback).Run(std::move(existing_transaction.value()));
  }

  switch (existing_transaction.error()) {
    case database::GetExternalTransactionError::kDatabaseError:
      return std::move(callback).Run(nullptr);
    case database::GetExternalTransactionError::kTransactionNotFound:
      break;
  }

  auto new_transaction = mojom::ExternalTransaction::New(
      "" /* transaction_id - to be generated */, std::move(contribution_id),
      std::move(destination), std::move(amount));

  CreateTransaction(base::BindOnce(&Transfer::SaveExternalTransaction,
                                   base::Unretained(this), std::move(callback)),
                    std::move(new_transaction));
}

void Transfer::SaveExternalTransaction(
    MaybeCreateTransactionCallback callback,
    mojom::ExternalTransactionPtr transaction) const {
  if (!transaction) {
    return std::move(callback).Run(nullptr);
  }

  DCHECK(!transaction->transaction_id.empty());

  auto on_save_external_transaction = base::BindOnce(
      &Transfer::OnSaveExternalTransaction, base::Unretained(this),
      std::move(callback), transaction->Clone());

  ledger_->database()->SaveExternalTransaction(
      std::move(transaction), std::move(on_save_external_transaction));
}

void Transfer::OnSaveExternalTransaction(
    MaybeCreateTransactionCallback callback,
    mojom::ExternalTransactionPtr transaction,
    mojom::Result result) const {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Failed to save external transaction!");
    return std::move(callback).Run(nullptr);
  }

  std::move(callback).Run(std::move(transaction));
}

void Transfer::CreateTransaction(
    MaybeCreateTransactionCallback callback,
    mojom::ExternalTransactionPtr transaction) const {
  DCHECK(transaction);
  DCHECK(transaction->transaction_id.empty());

  transaction->transaction_id = base::GenerateGUID();

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), std::move(transaction)));
}

}  // namespace ledger::wallet_provider
