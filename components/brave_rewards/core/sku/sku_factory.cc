/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/sku/sku_factory.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"

namespace brave_rewards::core {
namespace sku {

std::unique_ptr<SKU> SKUFactory::Create(LedgerImpl* ledger,
                                        const SKUType type) {
  DCHECK(ledger);

  switch (type) {
    case SKUType::kBrave: {
      return std::make_unique<SKUBrave>(ledger);
    }

    case SKUType::kMerchant: {
      return std::make_unique<SKUMerchant>(ledger);
    }
  }
}

}  // namespace sku
}  // namespace brave_rewards::core
