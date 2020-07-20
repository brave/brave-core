/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_SKU_ORDER_ITEMS_H_
#define BRAVELEDGER_DATABASE_DATABASE_SKU_ORDER_ITEMS_H_

#include <memory>
#include <string>

#include "bat/ledger/internal/database/database_table.h"

namespace braveledger_database {

using GetSKUOrderItemsCallback = std::function<void(ledger::SKUOrderItemList)>;

class DatabaseSKUOrderItems: public DatabaseTable {
 public:
  explicit DatabaseSKUOrderItems(bat_ledger::LedgerImpl* ledger);
  ~DatabaseSKUOrderItems() override;

  void InsertOrUpdateList(
      ledger::DBTransaction* transaction,
      ledger::SKUOrderItemList list);

  void GetRecordsByOrderId(
      const std::string& order_id,
      GetSKUOrderItemsCallback callback);

 private:
  void OnGetRecordsByOrderId(
      ledger::DBCommandResponsePtr response,
      GetSKUOrderItemsCallback callback);
};

}  // namespace braveledger_database

#endif  // BRAVELEDGER_DATABASE_DATABASE_SKU_ORDER_ITEMS_H_
