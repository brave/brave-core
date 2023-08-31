/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_SKU_ORDER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_SKU_ORDER_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_rewards/core/database/database_sku_order_items.h"
#include "brave/components/brave_rewards/core/database/database_table.h"

namespace brave_rewards::internal {
namespace database {

using GetSKUOrderCallback = std::function<void(mojom::SKUOrderPtr)>;

class DatabaseSKUOrder : public DatabaseTable {
 public:
  explicit DatabaseSKUOrder(RewardsEngineImpl& engine);
  ~DatabaseSKUOrder() override;

  void InsertOrUpdate(mojom::SKUOrderPtr info, LegacyResultCallback callback);

  void UpdateStatus(const std::string& order_id,
                    mojom::SKUOrderStatus status,
                    LegacyResultCallback callback);

  void GetRecord(const std::string& order_id, GetSKUOrderCallback callback);

  void GetRecordByContributionId(const std::string& contribution_id,
                                 GetSKUOrderCallback callback);

  void SaveContributionIdForSKUOrder(const std::string& order_id,
                                     const std::string& contribution_id,
                                     LegacyResultCallback callback);

 private:
  void OnGetRecord(GetSKUOrderCallback callback,
                   mojom::DBCommandResponsePtr response);

  void OnGetRecordItems(std::vector<mojom::SKUOrderItemPtr> list,
                        std::shared_ptr<mojom::SKUOrderPtr> shared_order,
                        GetSKUOrderCallback callback);

  DatabaseSKUOrderItems items_;
};

}  // namespace database
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_SKU_ORDER_H_
