/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_INITIALIZE_H_
#define BRAVELEDGER_DATABASE_DATABASE_INITIALIZE_H_

#include <memory>
#include <string>

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/database/database_migration.h"

namespace ledger {
class LedgerImpl;

namespace database {

class DatabaseInitialize {
 public:
  explicit DatabaseInitialize(LedgerImpl* ledger);
  ~DatabaseInitialize();

  void Start(
      const bool execute_create_script,
      ledger::ResultCallback callback);

 private:
  void OnInitialize(
      type::DBCommandResponsePtr response,
      const bool execute_create_script,
      ledger::ResultCallback callback);

  void EnsureCurrentVersion(
      const int table_version,
      ledger::ResultCallback callback);

  void GetCreateScript(ledger::ResultCallback callback);

  void ExecuteCreateScript(
      const std::string& script,
      const int table_version,
      ledger::ResultCallback callback);

  void OnExecuteCreateScript(
      type::DBCommandResponsePtr response,
      const int table_version,
      ledger::ResultCallback callback);

  std::unique_ptr<ledger::database::DatabaseMigration> migration_;
  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVELEDGER_DATABASE_DATABASE_INITIALIZE_H_
