/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_SKU_SKU_COMMON_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_SKU_SKU_COMMON_H_

#include <string>
#include <vector>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/brave_rewards/core/database/database_sku_transaction.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"
#include "brave/components/brave_rewards/core/sku/sku_order.h"
#include "brave/components/brave_rewards/core/sku/sku_transaction.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace sku {

class SKUCommon {
 public:
  explicit SKUCommon(RewardsEngineImpl& engine);
  ~SKUCommon();

  void CreateOrder(const std::vector<mojom::SKUOrderItem>& items,
                   SKUOrderCallback callback);

  void CreateTransaction(mojom::SKUOrderPtr order,
                         const std::string& destination,
                         const std::string& wallet_type,
                         SKUOrderCallback callback);

  void SendExternalTransaction(const std::string& order_id,
                               SKUOrderCallback callback);

 private:
  void OnTransactionCompleted(const std::string& order_id,
                              SKUOrderCallback callback,
                              mojom::Result result);

  void GetSKUTransactionByOrderId(
      SKUOrderCallback callback,
      base::expected<mojom::SKUTransactionPtr, database::GetSKUTransactionError>
          result);

  const raw_ref<RewardsEngineImpl> engine_;
  SKUOrder order_;
  SKUTransaction transaction_;
  base::WeakPtrFactory<SKUCommon> weak_factory_{this};
};

}  // namespace sku
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_SKU_SKU_COMMON_H_
