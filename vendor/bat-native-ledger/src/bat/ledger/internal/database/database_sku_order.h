/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_SKU_ORDER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_SKU_ORDER_H_

#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/internal/database/database_sku_order_items.h"
#include "bat/ledger/internal/database/database_table.h"

namespace ledger {
namespace database {

using GetSKUOrderCallback = std::function<void(mojom::SKUOrderPtr)>;

class DatabaseSKUOrder: public DatabaseTable {
 public:
  explicit DatabaseSKUOrder(LedgerImpl* ledger);
  ~DatabaseSKUOrder() override;

  void InsertOrUpdate(mojom::SKUOrderPtr info,
                      ledger::LegacyResultCallback callback);

  void UpdateStatus(const std::string& order_id,
                    mojom::SKUOrderStatus status,
                    ledger::LegacyResultCallback callback);

  void GetRecord(
      const std::string& order_id,
      GetSKUOrderCallback callback);

  void GetRecordByContributionId(
      const std::string& contribution_id,
      GetSKUOrderCallback callback);

  void SaveContributionIdForSKUOrder(const std::string& order_id,
                                     const std::string& contribution_id,
                                     ledger::LegacyResultCallback callback);

 private:
  void OnGetRecord(mojom::DBCommandResponsePtr response,
                   GetSKUOrderCallback callback);

  void OnGetRecordItems(std::vector<mojom::SKUOrderItemPtr> list,
                        std::shared_ptr<mojom::SKUOrderPtr> shared_order,
                        GetSKUOrderCallback callback);

  std::unique_ptr<DatabaseSKUOrderItems> items_;
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_SKU_ORDER_H_
