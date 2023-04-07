/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_SKU_SKU_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_SKU_SKU_H_

#include <map>
#include <string>
#include <vector>

#include "brave/components/brave_rewards/core/ledger.h"

namespace ledger {
namespace sku {

class SKU {
 public:
  virtual ~SKU() = default;

  virtual void Retry(const std::string& order_id,
                     const std::string& wallet_type,
                     ledger::SKUOrderCallback callback) = 0;

  virtual void Process(const std::vector<mojom::SKUOrderItem>& items,
                       const std::string& wallet_type,
                       ledger::SKUOrderCallback callback,
                       const std::string& contribution_id = "") = 0;
};

}  // namespace sku
}  // namespace ledger

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_SKU_SKU_H_
