/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/database/database_server_publisher_amounts.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace {

const char kTableName[] = "server_publisher_amounts";

}  // namespace

namespace braveledger_database {

DatabaseServerPublisherAmounts::DatabaseServerPublisherAmounts(
    bat_ledger::LedgerImpl* ledger) :
    DatabaseTable(ledger) {
}

DatabaseServerPublisherAmounts::~DatabaseServerPublisherAmounts() = default;

void DatabaseServerPublisherAmounts::InsertOrUpdate(
    ledger::DBTransaction* transaction,
    const ledger::ServerPublisherInfo& server_info) {
  DCHECK(transaction && !server_info.publisher_key.empty());
  if (!server_info.banner || server_info.banner->amounts.empty()) {
    return;
  }

  std::string value_list;
  for (const auto& amount : server_info.banner->amounts) {
    value_list += base::StringPrintf(
        R"(("%s",%g),)",
        server_info.publisher_key.c_str(),
        amount);
  }

  DCHECK(!value_list.empty());

  // Remove trailing comma
  value_list.pop_back();

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = base::StringPrintf(
      "INSERT OR REPLACE INTO %s VALUES %s",
      kTableName,
      value_list.c_str());

  transaction->commands.push_back(std::move(command));
}

void DatabaseServerPublisherAmounts::DeleteRecords(
    ledger::DBTransaction* transaction,
    const std::string& publisher_key_list) {
  DCHECK(transaction);
  if (publisher_key_list.empty()) {
    return;
  }

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = base::StringPrintf(
      "DELETE FROM %s WHERE publisher_key IN (%s)",
      kTableName,
      publisher_key_list.c_str());

  transaction->commands.push_back(std::move(command));
}

void DatabaseServerPublisherAmounts::GetRecord(
    const std::string& publisher_key,
    ServerPublisherAmountsCallback callback) {
  if (publisher_key.empty()) {
    BLOG(1, "Publisher key is empty");
    callback({});
    return;
  }
  auto transaction = ledger::DBTransaction::New();
  const std::string query = base::StringPrintf(
      "SELECT amount FROM %s WHERE publisher_key=?",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, publisher_key);

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabaseServerPublisherAmounts::OnGetRecord,
          this,
          _1,
          callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseServerPublisherAmounts::OnGetRecord(
    ledger::DBCommandResponsePtr response,
    ServerPublisherAmountsCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback({});
    return;
  }

  std::vector<double> amounts;
  for (auto const& record : response->result->get_records()) {
    amounts.push_back(GetDoubleColumn(record.get(), 0));
  }

  callback(amounts);
}

}  // namespace braveledger_database
