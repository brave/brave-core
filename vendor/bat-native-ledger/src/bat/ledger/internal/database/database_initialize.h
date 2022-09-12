/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_INITIALIZE_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_INITIALIZE_H_

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

  void Start(bool execute_create_script, ledger::LegacyResultCallback callback);

 private:
  void OnInitialize(mojom::DBCommandResponsePtr response,
                    bool execute_create_script,
                    ledger::LegacyResultCallback callback);

  void GetCreateScript(ledger::LegacyResultCallback callback);

  void ExecuteCreateScript(const std::string& script,
                           int table_version,
                           ledger::LegacyResultCallback callback);

  void OnExecuteCreateScript(mojom::DBCommandResponsePtr response,
                             int table_version,
                             ledger::LegacyResultCallback callback);

  std::unique_ptr<ledger::database::DatabaseMigration> migration_;
  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_INITIALIZE_H_
