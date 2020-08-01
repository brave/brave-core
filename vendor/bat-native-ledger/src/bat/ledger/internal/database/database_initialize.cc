/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/internal/database/database_initialize.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/state/state_keys.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace braveledger_database {

DatabaseInitialize::DatabaseInitialize(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger) {
  migration_ =
      std::make_unique<ledger::database::DatabaseMigration>(ledger_);
}

DatabaseInitialize::~DatabaseInitialize() = default;

void DatabaseInitialize::Start(
    const bool execute_create_script,
    ledger::ResultCallback callback) {
  auto transaction = ledger::DBTransaction::New();
  transaction->version = GetCurrentVersion();
  transaction->compatible_version = GetCompatibleVersion();
  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::INITIALIZE;
  transaction->commands.push_back(std::move(command));

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      std::bind(&DatabaseInitialize::OnInitialize,
          this,
          _1,
          execute_create_script,
          callback));
}

void DatabaseInitialize::OnInitialize(
    ledger::DBCommandResponsePtr response,
    const bool execute_create_script,
    ledger::ResultCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback(ledger::Result::DATABASE_INIT_FAILED);
    return;
  }

  if (execute_create_script) {
    GetCreateScript(callback);
    return;
  }

  if (!response->result ||
      response->result->get_value()->which() !=
      ledger::DBValue::Tag::INT_VALUE) {
    BLOG(0, "DB init failed");
    callback(ledger::Result::DATABASE_INIT_FAILED);
    return;
  }

  const auto current_table_version =
      response->result->get_value()->get_int_value();
  migration_->Start(current_table_version, callback);
}

void DatabaseInitialize::GetCreateScript(ledger::ResultCallback callback) {
  auto script_callback = std::bind(&DatabaseInitialize::ExecuteCreateScript,
      this,
      _1,
      _2,
      callback);
  ledger_->ledger_client()->GetCreateScript(script_callback);
}

void DatabaseInitialize::ExecuteCreateScript(
    const std::string& script,
    const int table_version,
    ledger::ResultCallback callback) {
  if (script.empty()) {
    BLOG(1, "Script is empty");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  ledger_->ledger_client()->ClearState(ledger::kStateServerPublisherListStamp);

  auto script_callback = std::bind(&DatabaseInitialize::OnExecuteCreateScript,
      this,
      _1,
      table_version,
      callback);

  auto transaction = ledger::DBTransaction::New();
  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = script;
  transaction->commands.push_back(std::move(command));

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      script_callback);
}

void DatabaseInitialize::OnExecuteCreateScript(
    ledger::DBCommandResponsePtr response,
    const int table_version,
    ledger::ResultCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback(ledger::Result::DATABASE_INIT_FAILED);
    return;
  }

  migration_->Start(table_version, callback);
}

}  // namespace braveledger_database
