/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_SKU_SKU_ORDER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_SKU_SKU_ORDER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/internal/endpoint/payment/payment_server.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace sku {

class SKUOrder {
 public:
  explicit SKUOrder(LedgerImpl* ledger);
  ~SKUOrder();

  void Create(
      std::vector<mojom::SKUOrderItemPtr> items,
      ledger::SKUOrderCallback callback);

 private:
  void OnCreate(
      const type::Result result,
      type::SKUOrderPtr order,
      ledger::SKUOrderCallback callback);

  void OnCreateSave(
      const type::Result result,
      const std::string& order_id,
      ledger::SKUOrderCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<endpoint::PaymentServer> payment_server_;
};

}  // namespace sku
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_SKU_SKU_ORDER_H_
