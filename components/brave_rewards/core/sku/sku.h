/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_SKU_SKU_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_SKU_SKU_H_

#include <string>
#include <vector>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"
#include "brave/components/brave_rewards/core/sku/sku_common.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace sku {

class SKU {
 public:
  explicit SKU(RewardsEngineImpl& engine);
  ~SKU();

  void Process(const std::vector<mojom::SKUOrderItem>& items,
               const std::string& wallet_type,
               SKUOrderCallback callback,
               const std::string& contribution_id = "");

  void Retry(const std::string& order_id,
             const std::string& wallet_type,
             SKUOrderCallback callback);

 private:
  void OrderCreated(const std::string& wallet_type,
                    const std::string& contribution_id,
                    SKUOrderCallback callback,
                    mojom::Result result,
                    const std::string& order_id);

  void ContributionIdSaved(const std::string& order_id,
                           const std::string& wallet_type,
                           SKUOrderCallback callback,
                           mojom::Result result);

  void CreateTransaction(const std::string& wallet_type,
                         SKUOrderCallback callback,
                         mojom::SKUOrderPtr order);

  void OnOrder(const std::string& wallet_type,
               SKUOrderCallback callback,
               mojom::SKUOrderPtr order);

  const raw_ref<RewardsEngineImpl> engine_;
  SKUCommon common_;
  base::WeakPtrFactory<SKU> weak_factory_{this};
};

}  // namespace sku
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_SKU_SKU_H_
