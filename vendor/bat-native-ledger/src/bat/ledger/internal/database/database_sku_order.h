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

namespace ledger {
namespace database {

using GetSKUOrderCallback = std::function<void(type::SKUOrderPtr)>;

class DatabaseSKUOrder: public DatabaseTable {
 public:
  explicit DatabaseSKUOrder(LedgerImpl* ledger);
  ~DatabaseSKUOrder() override;

  void InsertOrUpdate(
      type::SKUOrderPtr info,
      ledger::ResultCallback callback);

  void UpdateStatus(
      const std::string& order_id,
      const type::SKUOrderStatus status,
      ledger::ResultCallback callback);

  void GetRecord(
      const std::string& order_id,
      GetSKUOrderCallback callback);

  void GetRecordByContributionId(
      const std::string& contribution_id,
      GetSKUOrderCallback callback);

  void SaveContributionIdForSKUOrder(
      const std::string& order_id,
      const std::string& contribution_id,
      ledger::ResultCallback callback);

 private:
  void OnGetRecord(
      type::DBCommandResponsePtr response,
      GetSKUOrderCallback callback);

  void OnGetRecordItems(
      type::SKUOrderItemList list,
      std::shared_ptr<type::SKUOrderPtr> shared_order,
      GetSKUOrderCallback callback);

  std::unique_ptr<DatabaseSKUOrderItems> items_;
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVELEDGER_DATABASE_DATABASE_ORDER_H_
