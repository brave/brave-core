/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/internal/database/database_initialize.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace {

const int kCurrentVersionNumber = 15;
const int kCompatibleVersionNumber = 1;

}  // namespace

namespace braveledger_database {

DatabaseInitialize::DatabaseInitialize(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger) {
}

DatabaseInitialize::~DatabaseInitialize() = default;

void DatabaseInitialize::Start(
    const bool execute_create_script,
    ledger::ResultCallback callback) {
  auto transaction = ledger::DBTransaction::New();
  transaction->version = kCurrentVersionNumber;
  transaction->compatible_version = kCompatibleVersionNumber;

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::INITIALIZE;
  transaction->commands.push_back(std::move(command));

  ledger_->RunDBTransaction(
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
    callback(ledger::Result::DATABASE_INIT_FAILED);
    return;
  }

  const auto current_table_version =
      response->result->get_value()->get_int_value();
  EnsureCurrentVersion(current_table_version, callback);
}

void DatabaseInitialize::EnsureCurrentVersion(
    const int table_version,
    ledger::ResultCallback callback) {
  callback(ledger::Result::LEDGER_OK);
}

void DatabaseInitialize::GetCreateScript(ledger::ResultCallback callback) {
  auto script_callback = std::bind(&DatabaseInitialize::ExecuteCreateScript,
      this,
      _1,
      _2,
      callback);
  ledger_->GetCreateScript(script_callback);
}

void DatabaseInitialize::ExecuteCreateScript(
    const std::string& script,
    const int table_version,
    ledger::ResultCallback callback) {
  if (script.empty()) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

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

  ledger_->RunDBTransaction(std::move(transaction), script_callback);
}

void DatabaseInitialize::OnExecuteCreateScript(
    ledger::DBCommandResponsePtr response,
    const int table_version,
    ledger::ResultCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    callback(ledger::Result::DATABASE_INIT_FAILED);
    return;
  }

  EnsureCurrentVersion(table_version, callback);
}

}  // namespace braveledger_database
