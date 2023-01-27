/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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
      // We can one-time/monthly tip in multiples of 0.25.
      // A-C we can do in multiples of 1 (which is also a multiple of 0.25).
      // The least amount one can tip is 0.25, where:
      //   - 0.2375 (95%) is the actual contribution, and
      //   - 0.0125 (== 1/80) (5%) is the fee
      // Rounding to nearest eightieth:
      base::NumberToString(std::round(amount * 80) / 80),
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
    mojom::ExternalTransactionPtr transaction) const {
  if (transaction) {
    return std::move(callback).Run(std::move(transaction));
  }

  transaction = mojom::ExternalTransaction::New(
      "" /* transaction_id - to be generated */, std::move(contribution_id),
      std::move(destination), std::move(amount));

  CreateTransaction(base::BindOnce(&Transfer::SaveExternalTransaction,
                                   base::Unretained(this), std::move(callback)),
                    std::move(transaction));
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

  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), std::move(transaction)));
}

}  // namespace ledger::wallet_provider
