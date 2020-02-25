/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_TABLE_H_
#define BRAVELEDGER_DATABASE_DATABASE_TABLE_H_

#include <string>

#include "bat/ledger/ledger.h"
#include "sql/database.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_database {

class DatabaseTable {
 public:
  explicit DatabaseTable(bat_ledger::LedgerImpl* ledger);
  virtual ~DatabaseTable();

  virtual bool Migrate(
      ledger::DBTransaction* transaction,
      const int target) = 0;

 protected:
  bool InsertIndex(
      ledger::DBTransaction* transaction,
      const std::string& table_name,
      const std::string& key);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_database

#endif  // BRAVELEDGER_DATABASE_DATABASE_TABLE_H_
