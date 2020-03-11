/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_ORDER_H_
#define BRAVELEDGER_DATABASE_DATABASE_ORDER_H_

#include <memory>
#include <string>

#include "bat/ledger/internal/database/database_sku_order_items.h"
#include "bat/ledger/internal/database/database_table.h"

namespace braveledger_database {

class DatabaseSKUOrder: public DatabaseTable {
 public:
  explicit DatabaseSKUOrder(bat_ledger::LedgerImpl* ledger);
  ~DatabaseSKUOrder() override;

  bool Migrate(ledger::DBTransaction* transaction, const int target) override;

  void InsertOrUpdate(
      ledger::SKUOrderPtr info,
      ledger::ResultCallback callback);

  void UpdateStatus(
      const std::string& order_id,
      const ledger::SKUOrderStatus status,
      ledger::ResultCallback callback);

  void GetRecord(
      const std::string& order_id,
      ledger::GetSKUOrderCallback callback);

 private:
  bool CreateTableV19(ledger::DBTransaction* transaction);

  bool MigrateToV19(ledger::DBTransaction* transaction);

  void OnGetRecord(
      ledger::DBCommandResponsePtr response,
      ledger::GetSKUOrderCallback callback);

  void OnGetRecordItems(
      ledger::SKUOrderItemList list,
      const std::string& order_string,
      ledger::GetSKUOrderCallback callback);

  std::unique_ptr<DatabaseSKUOrderItems> items_;
};

}  // namespace braveledger_database

#endif  // BRAVELEDGER_DATABASE_DATABASE_ORDER_H_
