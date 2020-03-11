/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_SKU_TRANSACTION_H_
#define BRAVELEDGER_DATABASE_DATABASE_SKU_TRANSACTION_H_

#include <string>

#include "bat/ledger/internal/database/database_table.h"

namespace braveledger_database {

class DatabaseSKUTransaction: public DatabaseTable {
 public:
  explicit DatabaseSKUTransaction(bat_ledger::LedgerImpl* ledger);
  ~DatabaseSKUTransaction() override;

  bool Migrate(ledger::DBTransaction* transaction, const int target) override;

  void InsertOrUpdate(
      ledger::SKUTransactionPtr info,
      ledger::ResultCallback callback);

  void SaveExternalTransaction(
      const std::string& transaction_id,
      const std::string& external_transaction_id,
      ledger::ResultCallback callback);

 private:
  bool CreateTableV19(ledger::DBTransaction* transaction);

  bool CreateIndexV19(ledger::DBTransaction* transaction);

  bool MigrateToV19(ledger::DBTransaction* transaction);
};

}  // namespace braveledger_database

#endif  // BRAVELEDGER_DATABASE_DATABASE_SKU_TRANSACTION_H_
