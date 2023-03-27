/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_SKU_SKU_MERCHANT_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_SKU_SKU_MERCHANT_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_rewards/core/ledger.h"
#include "brave/components/brave_rewards/core/sku/sku.h"
#include "brave/components/brave_rewards/core/sku/sku_common.h"

namespace brave_rewards::core {
class LedgerImpl;

namespace sku {

class SKUMerchant : public SKU {
 public:
  explicit SKUMerchant(LedgerImpl* ledger);
  ~SKUMerchant() override;

  void Process(const std::vector<mojom::SKUOrderItem>& items,
               const std::string& wallet_type,
               SKUOrderCallback callback,
               const std::string& contribution_id = "") override;

  void Retry(const std::string& order_id,
             const std::string& wallet_type,
             SKUOrderCallback callback) override;

 private:
  void OrderCreated(const mojom::Result result,
                    const std::string& order_id,
                    const std::string& wallet_type,
                    SKUOrderCallback callback);

  void OnOrder(mojom::SKUOrderPtr order,
               const std::string& wallet_type,
               SKUOrderCallback callback);

  void OnServerPublisherInfo(mojom::ServerPublisherInfoPtr info,
                             std::shared_ptr<mojom::SKUOrderPtr> shared_order,
                             const std::string& wallet_type,
                             SKUOrderCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<SKUCommon> common_;
};

}  // namespace sku
}  // namespace brave_rewards::core

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_SKU_SKU_MERCHANT_H_
