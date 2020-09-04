/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_SKU_MERCHANT_H_
#define BRAVELEDGER_SKU_MERCHANT_H_

#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/internal/sku/sku.h"
#include "bat/ledger/internal/sku/sku_common.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace sku {

class SKUMerchant : public SKU  {
 public:
  explicit SKUMerchant(LedgerImpl* ledger);
  ~SKUMerchant() override;

  void Process(
      const std::vector<type::SKUOrderItem>& items,
      type::ExternalWalletPtr wallet,
      ledger::SKUOrderCallback callback,
      const std::string& contribution_id = "") override;

  void Retry(
      const std::string& order_id,
      type::ExternalWalletPtr wallet,
      ledger::SKUOrderCallback callback) override;

 private:
  void OrderCreated(
      const type::Result result,
      const std::string& order_id,
      const type::ExternalWallet& wallet,
      ledger::SKUOrderCallback callback);

  void OnOrder(
      type::SKUOrderPtr order,
      const type::ExternalWallet& wallet,
      ledger::SKUOrderCallback callback);

  void OnServerPublisherInfo(
      type::ServerPublisherInfoPtr info,
      std::shared_ptr<type::SKUOrderPtr> shared_order,
      const type::ExternalWallet& wallet,
      ledger::SKUOrderCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<SKUCommon> common_;
};

}  // namespace sku
}  // namespace ledger

#endif  // BRAVELEDGER_SKU_MERCHANT_H_
