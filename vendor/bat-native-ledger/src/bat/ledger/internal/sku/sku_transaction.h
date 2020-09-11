/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_SKU_TRANSACTION_H_
#define BRAVELEDGER_SKU_TRANSACTION_H_

#include <map>
#include <memory>
#include <string>

#include "bat/ledger/internal/endpoint/payment/payment_server.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace sku {

class SKUTransaction {
 public:
  explicit SKUTransaction(LedgerImpl* ledger);
  ~SKUTransaction();

  void Create(
      type::SKUOrderPtr order,
      const std::string& destination,
      const std::string& wallet_type,
      ledger::ResultCallback callback);

  void SendExternalTransaction(
      const type::Result result,
      const type::SKUTransaction& transaction,
      ledger::ResultCallback callback);

 private:
  void OnTransactionSaved(
      const type::Result result,
      const type::SKUTransaction& transaction,
      const std::string& destination,
      const std::string& wallet_type,
      ledger::ResultCallback callback);

  void OnTransfer(
      const type::Result result,
      const std::string& external_transaction_id,
      const type::SKUTransaction& transaction,
      ledger::ResultCallback callback);

  void OnSaveSKUExternalTransaction(
      const type::Result result,
      const type::SKUTransaction& transaction,
      ledger::ResultCallback callback);

  void OnSendExternalTransaction(
      const type::Result result,
      ledger::ResultCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<endpoint::PaymentServer> payment_server_;
};

}  // namespace sku
}  // namespace ledger

#endif  // BRAVELEDGER_SKU_TRANSACTION_H_
