/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

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
    type::ExternalTransactionPtr external_transaction,
    ledger::ResultCallback callback) {
  if (!external_transaction) {
    BLOG(0, "external_transaction is null!");
    return std::move(callback).Run(type::Result::LEDGER_ERROR);
  }

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::RUN;
  command->command = base::StringPrintf(
      R"(
        INSERT INTO %s (wallet_provider, transaction_id, contribution_id, is_fee, status)
        VALUES(?, ?, ?, ?, ?)
      )",
      kTableName);
  BindInt(command.get(), 0,
          static_cast<std::underlying_type_t<type::WalletProvider>>(
              external_transaction->wallet_provider));
  BindString(command.get(), 1, external_transaction->transaction_id);
  BindString(command.get(), 2, external_transaction->contribution_id);
  BindBool(command.get(), 3, external_transaction->is_fee);
  BindInt(command.get(), 4,
          static_cast<std::underlying_type_t<type::ExternalTransactionStatus>>(
              external_transaction->status));

  auto transaction = type::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  ledger_->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback<>, std::move(callback)));
}

void DatabaseExternalTransactions::GetExternalTransactionId(
    const std::string& contribution_id,
    bool is_fee,
    GetExternalTransactionIdCallback callback) {
  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::READ;
  command->command = base::StringPrintf(
      R"(
        SELECT (wallet_provider, transaction_id, contribution_id, is_fee, status)
        FROM %s
        WHERE contribution_id = ? AND is_fee = ?
      )",
      kTableName);
  command->record_bindings = {type::DBCommand::RecordBindingType::INT_TYPE,
                              type::DBCommand::RecordBindingType::STRING_TYPE,
                              type::DBCommand::RecordBindingType::STRING_TYPE,
                              type::DBCommand::RecordBindingType::BOOL_TYPE,
                              type::DBCommand::RecordBindingType::INT_TYPE};
  BindString(command.get(), 0, contribution_id);
  BindBool(command.get(), 1, is_fee);

  auto transaction = type::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  ledger_->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseExternalTransactions::OnGetExternalTransactionId,
                     base::Unretained(this), std::move(callback)));
}

void DatabaseExternalTransactions::OnGetExternalTransactionId(
    GetExternalTransactionIdCallback callback,
    type::DBCommandResponsePtr response) {
  if (!response ||
      response->status != type::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Failed to get external transaction ID!");
    return std::move(callback).Run(absl::nullopt);
  }

  const auto& records = response->result->get_records();
  if (records.empty()) {
    return std::move(callback).Run("");
  }

  DCHECK(records.size() == 1);

  auto transaction_id = GetStringColumn(records[0].get(), 0);
  if (transaction_id.empty()) {
    BLOG(0, "Failed to get external transaction ID!");
    return std::move(callback).Run(absl::nullopt);
  }

  std::move(callback).Run(std::move(transaction_id));
}

}  // namespace ledger::database
