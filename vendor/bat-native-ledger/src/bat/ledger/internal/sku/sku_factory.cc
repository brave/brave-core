/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/sku/sku_factory.h"
#include "bat/ledger/internal/ledger_impl.h"

namespace braveledger_sku {

std::unique_ptr<SKU> SKUFactory::Create(
    bat_ledger::LedgerImpl* ledger,
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

}  // namespace braveledger_sku
