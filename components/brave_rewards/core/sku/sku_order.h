/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_SKU_SKU_ORDER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_SKU_SKU_ORDER_H_

#include <map>
#include <string>
#include <vector>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/core/endpoint/payment/payment_server.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace sku {

class SKUOrder {
 public:
  explicit SKUOrder(RewardsEngineImpl& engine);
  ~SKUOrder();

  void Create(const std::vector<mojom::SKUOrderItem>& items,
              SKUOrderCallback callback);

 private:
  void OnCreate(const mojom::Result result,
                mojom::SKUOrderPtr order,
                SKUOrderCallback callback);

  void OnCreateSave(const mojom::Result result,
                    const std::string& order_id,
                    SKUOrderCallback callback);

  const raw_ref<RewardsEngineImpl> engine_;
  endpoint::PaymentServer payment_server_;
};

}  // namespace sku
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_SKU_SKU_ORDER_H_
