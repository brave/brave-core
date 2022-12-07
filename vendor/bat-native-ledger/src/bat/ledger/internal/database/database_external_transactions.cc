/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/database/database_external_transactions.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

namespace ledger::database {

constexpr char kTableName[] = "external_transactions";

DatabaseExternalTransactions::DatabaseExternalTransactions(LedgerImpl* ledger)
    : DatabaseTable(ledger) {}

DatabaseExternalTransactions::~DatabaseExternalTransactions() = default;

void DatabaseExternalTransactions::Insert(
    mojom::ExternalTransactionPtr external_transaction,
    ledger::ResultCallback callback) {
  if (!external_transaction) {
    BLOG(0, "external_transaction is null!");
    return std::move(callback).Run(mojom::Result::LEDGER_ERROR);
  }

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = base::StringPrintf(
      R"(
        INSERT INTO %s (transaction_id, contribution_id, destination, amount)
        VALUES(?, ?, ?, ?)
      )",
      kTableName);
  BindString(command.get(), 0, external_transaction->transaction_id);
  BindString(command.get(), 1, external_transaction->contribution_id);
  BindString(command.get(), 2, external_transaction->destination);
  BindDouble(command.get(), 3, external_transaction->amount);

  auto transaction = mojom::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  ledger_->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseExternalTransactions::OnInsert,
                     base::Unretained(this), std::move(callback)));
}

void DatabaseExternalTransactions::OnInsert(
    ledger::ResultCallback callback,
    mojom::DBCommandResponsePtr response) {
  std::move(callback).Run(
      response &&
              response->status == mojom::DBCommandResponse::Status::RESPONSE_OK
          ? mojom::Result::LEDGER_OK
          : mojom::Result::LEDGER_ERROR);
}

void DatabaseExternalTransactions::GetTransactionId(
    const std::string& contribution_id,
    const std::string& destination,
    GetExternalTransactionIdCallback callback) {
  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = base::StringPrintf(
      R"(
        SELECT transaction_id
        FROM %s
        WHERE contribution_id = ? AND destination = ?
      )",
      kTableName);
  command->record_bindings = {mojom::DBCommand::RecordBindingType::STRING_TYPE};
  BindString(command.get(), 0, contribution_id);
  BindString(command.get(), 1, destination);

  auto transaction = mojom::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  ledger_->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseExternalTransactions::OnGetTransactionId,
                     base::Unretained(this), std::move(callback)));
}

void DatabaseExternalTransactions::OnGetTransactionId(
    GetExternalTransactionIdCallback callback,
    mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Failed to get external transaction ID!");
    return std::move(callback).Run("");
  }

  const auto& records = response->result->get_records();
  if (records.empty()) {
    return std::move(callback).Run("");
  }

  DCHECK_EQ(records.size(), 1ull);

  auto transaction_id = GetStringColumn(records[0].get(), 0);
  if (transaction_id.empty()) {
    BLOG(0, "Failed to get external transaction ID!");
    return std::move(callback).Run("");
  }

  std::move(callback).Run(std::move(transaction_id));
}

}  // namespace ledger::database
