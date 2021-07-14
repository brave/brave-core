/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_INCLUDE_BAT_LEDGER_LEDGER_DATABASE_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_INCLUDE_BAT_LEDGER_LEDGER_DATABASE_H_

#include <memory>

#include "base/files/file_path.h"
#include "base/memory/memory_pressure_listener.h"
#include "base/sequence_checker.h"
#include "bat/ledger/export.h"
#include "brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger_database.mojom.h"
#include "sql/database.h"
#include "sql/meta_table.h"

namespace ledger {

class LEDGER_EXPORT LedgerDatabase {
 public:
  explicit LedgerDatabase(const base::FilePath& path);

  LedgerDatabase(const LedgerDatabase&) = delete;
  LedgerDatabase& operator=(const LedgerDatabase&) = delete;

  ~LedgerDatabase();

  // Synchronously runs the specified list of commands in a transaction and
  // returns a database response object.
  mojom::DBCommandResponsePtr RunTransaction(
      mojom::DBTransactionPtr transaction);

  // Closes the database and deletes the underlying SQLite database file and any
  // temporary files.
  bool DeleteDatabaseFile();

  sql::Database* GetInternalDatabaseForTesting() { return &db_; }

 private:
  mojom::DBCommandResponse::Status Initialize(
      int32_t version,
      int32_t compatible_version,
      mojom::DBCommandResponse* command_response);

  mojom::DBCommandResponse::Status Execute(mojom::DBCommand* command);

  mojom::DBCommandResponse::Status Run(mojom::DBCommand* command);

  mojom::DBCommandResponse::Status Read(
      mojom::DBCommand* command,
      mojom::DBCommandResponse* command_response);

  mojom::DBCommandResponse::Status Migrate(int32_t version,
                                           int32_t compatible_version);

  void CloseDatabase();

  void OnMemoryPressure(
      base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level);

  const base::FilePath db_path_;
  sql::Database db_;
  sql::MetaTable meta_table_;
  std::unique_ptr<base::MemoryPressureListener> memory_pressure_listener_;
  bool initialized_ = false;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_INCLUDE_BAT_LEDGER_LEDGER_DATABASE_H_
