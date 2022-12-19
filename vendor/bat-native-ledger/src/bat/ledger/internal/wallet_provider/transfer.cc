/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet_provider/transfer.h"

#include <utility>

#include "base/guid.h"
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
      contribution_id, destination, amount,
      base::BindOnce(&Transfer::CommitTransaction, base::Unretained(this),
                     std::move(callback), destination, amount));
}

void Transfer::MaybeCreateTransaction(
    const std::string& contribution_id,
    const std::string& destination,
    double amount,
    MaybeCreateTransactionCallback callback) const {
  ledger_->database()->GetExternalTransactionId(
      contribution_id, destination,
      base::BindOnce(&Transfer::OnGetExternalTransactionId,
                     base::Unretained(this), std::move(callback),
                     contribution_id, destination, amount));
}

void Transfer::OnGetExternalTransactionId(
    MaybeCreateTransactionCallback callback,
    std::string&& contribution_id,
    std::string&& destination,
    double amount,
    std::string&& transaction_id) const {
  if (!transaction_id.empty()) {
    return std::move(callback).Run(std::move(transaction_id));
  }

  auto create_transaction_callback = base::BindOnce(
      &Transfer::SaveExternalTransaction, base::Unretained(this),
      std::move(callback), std::move(contribution_id), destination, amount);

  CreateTransaction(std::move(create_transaction_callback),
                    std::move(destination), amount);
}

void Transfer::SaveExternalTransaction(MaybeCreateTransactionCallback callback,
                                       std::string&& contribution_id,
                                       std::string&& destination,
                                       double amount,
                                       std::string&& transaction_id) const {
  if (transaction_id.empty()) {
    return std::move(callback).Run("");
  }

  auto external_transaction = mojom::ExternalTransaction::New(
      transaction_id, std::move(contribution_id), std::move(destination),
      amount);

  ledger_->database()->SaveExternalTransaction(
      std::move(external_transaction),
      base::BindOnce(&Transfer::OnSaveExternalTransaction,
                     base::Unretained(this), std::move(callback),
                     std::move(transaction_id)));
}

void Transfer::OnSaveExternalTransaction(
    MaybeCreateTransactionCallback callback,
    std::string&& transaction_id,
    mojom::Result result) const {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Failed to save external transaction ID!");
    return std::move(callback).Run("");
  }

  std::move(callback).Run(std::move(transaction_id));
}

void Transfer::CreateTransaction(MaybeCreateTransactionCallback callback,
                                 std::string&&,
                                 double) const {
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), base::GenerateGUID()));
}

}  // namespace ledger::wallet_provider
