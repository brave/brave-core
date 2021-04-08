/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_SKU_SKU_COMMON_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_SKU_SKU_COMMON_H_

#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/internal/sku/sku_order.h"
#include "bat/ledger/internal/sku/sku_transaction.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace sku {

class SKUCommon {
 public:
  explicit SKUCommon(LedgerImpl* ledger);
  ~SKUCommon();

  void CreateOrder(
      std::vector<mojom::SKUOrderItemPtr> items,
      ledger::SKUOrderCallback callback);

  void CreateTransaction(
      type::SKUOrderPtr order,
      const std::string& destination,
      const std::string& wallet_type,
      ledger::SKUOrderCallback callback);

  void SendExternalTransaction(
      const std::string& order_id,
      ledger::SKUOrderCallback callback);

 private:
  void OnTransactionCompleted(
      const type::Result result,
      const std::string& order_id,
      ledger::SKUOrderCallback callback);

  void GetSKUTransactionByOrderId(
      type::SKUTransactionPtr transaction,
      ledger::SKUOrderCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<SKUOrder> order_;
  std::unique_ptr<SKUTransaction> transaction_;
};

}  // namespace sku
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_SKU_SKU_COMMON_H_
