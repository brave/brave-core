/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_SKU_H_
#define BRAVELEDGER_SKU_H_

#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/internal/sku/sku_order.h"
#include "bat/ledger/internal/sku/sku_transaction.h"
#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_sku {

class SKU {
 public:
  explicit SKU(bat_ledger::LedgerImpl* ledger);
  ~SKU();

  void Brave(
      const std::string& destination,
      const std::vector<ledger::SKUOrderItem>& items,
      const std::string& contribution_id,
      ledger::ExternalWalletPtr wallet,
      ledger::SKUOrderCallback callback);

  void Process(
      const std::vector<ledger::SKUOrderItem>& items,
      ledger::ExternalWalletPtr wallet,
      ledger::SKUOrderCallback callback);

 private:
  void GetOrder(
      const ledger::Result result,
      const std::string& order_id,
      const std::string& destination,
      const ledger::ExternalWallet& wallet,
      ledger::SKUOrderCallback callback);

  void CreateTransaction(
      ledger::SKUOrderPtr order,
      const std::string& destination,
      const ledger::ExternalWallet& wallet,
      ledger::SKUOrderCallback callback);

  void OnServerPublisherInfo(
      ledger::ServerPublisherInfoPtr info,
      const std::string& order_string,
      const ledger::ExternalWallet& wallet,
      ledger::SKUOrderCallback callback);

  void OnTransactionCompleted(
      const ledger::Result result,
      const std::string& order_id,
      ledger::SKUOrderCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<SKUOrder> order_;
  std::unique_ptr<SKUTransaction> transaction_;
};

}  // namespace braveledger_sku

#endif  // BRAVELEDGER_SKU_H_
