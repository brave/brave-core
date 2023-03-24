/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_SKU_SKU_FACTORY_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_SKU_SKU_FACTORY_H_

#include <memory>

#include "brave/components/brave_rewards/core/sku/sku.h"
#include "brave/components/brave_rewards/core/sku/sku_brave.h"
#include "brave/components/brave_rewards/core/sku/sku_merchant.h"

namespace brave_rewards::core {
class LedgerImpl;

namespace sku {

enum SKUType { kBrave = 0, kMerchant = 1 };

class SKUFactory {
 public:
  static std::unique_ptr<SKU> Create(LedgerImpl* ledger, const SKUType type);
};

}  // namespace sku
}  // namespace brave_rewards::core

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_SKU_SKU_FACTORY_H_
