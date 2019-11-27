/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_H_
#define BRAVELEDGER_DATABASE_DATABASE_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/memory/memory_pressure_listener.h"
#include "base/sequence_checker.h"
#include "bat/ledger/ledger.h"
#include "sql/database.h"
#include "sql/meta_table.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_database {

class Database {
 public:
  explicit Database(bat_ledger::LedgerImpl* ledger);

  ~Database();

  // Call before Init() to set the error callback to be used for the
  // underlying database connection.
  void set_error_callback(const sql::Database::ErrorCallback& error_callback) {
    db_.set_error_callback(error_callback);
  }

  void Initialize();

  int GetCurrentVersion();

 private:
  void OnMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level);

  bool InitializeTables();

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  sql::Database db_;
  sql::MetaTable meta_table_;
  std::unique_ptr<base::MemoryPressureListener> memory_pressure_listener_;
  int testing_current_version_;

  SEQUENCE_CHECKER(sequence_checker_);
  DISALLOW_COPY_AND_ASSIGN(Database);
};

}  // namespace braveledger_database
#endif  // BRAVELEDGER_DATABASE_DATABASE_H_
