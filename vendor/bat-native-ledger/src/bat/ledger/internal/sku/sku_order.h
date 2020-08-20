/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_SKU_ORDER_H_
#define BRAVELEDGER_SKU_ORDER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/internal/endpoint/payment/payment_server.h"
#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_sku {

class SKUOrder {
 public:
  explicit SKUOrder(bat_ledger::LedgerImpl* ledger);
  ~SKUOrder();

  void Create(
      const std::vector<ledger::SKUOrderItem>& items,
      ledger::SKUOrderCallback callback);

 private:
  void OnCreate(
      const ledger::Result result,
      ledger::SKUOrderPtr order,
      ledger::SKUOrderCallback callback);

  void OnCreateSave(
      const ledger::Result result,
      const std::string& order_id,
      ledger::SKUOrderCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<ledger::endpoint::PaymentServer> payment_server_;
};

}  // namespace braveledger_sku

#endif  // BRAVELEDGER_SKU_ORDER_H_
