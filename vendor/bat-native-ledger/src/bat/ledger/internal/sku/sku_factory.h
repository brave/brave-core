/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_SKU_FACTORY_H_
#define BRAVELEDGER_SKU_FACTORY_H_

#include <memory>

#include "bat/ledger/internal/sku/sku.h"
#include "bat/ledger/internal/sku/sku_brave.h"
#include "bat/ledger/internal/sku/sku_merchant.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_sku {

enum SKUType {
  kBrave = 0,
  kMerchant = 1
};

class SKUFactory {
 public:
  static std::unique_ptr<SKU> Create(
      bat_ledger::LedgerImpl* ledger,
      const SKUType type);
};

}  // namespace braveledger_sku

#endif  // BRAVELEDGER_SKU_FACTORY_H_
