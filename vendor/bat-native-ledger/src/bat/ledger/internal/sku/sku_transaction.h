/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_SKU_TRANSACTION_H_
#define BRAVELEDGER_SKU_TRANSACTION_H_

#include <map>
#include <string>

#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_sku {

class SKUTransaction {
 public:
  explicit SKUTransaction(bat_ledger::LedgerImpl* ledger);
  ~SKUTransaction();

  void Create(
      ledger::SKUOrderPtr order,
      const std::string& destination,
      const ledger::ExternalWallet& wallet,
      ledger::ResultCallback callback);

 private:
  void OnTransactionSaved(
      const ledger::Result result,
      const ledger::SKUTransaction& transaction,
      const std::string& destination,
      const ledger::ExternalWallet& wallet,
      ledger::ResultCallback callback);

  void OnTransfer(
      const ledger::Result result,
      const std::string& external_transaction_id,
      const ledger::SKUTransaction& transaction,
      ledger::ResultCallback callback);

  void OnSaveSKUExternalTransaction(
      const ledger::Result result,
      const ledger::SKUTransaction& transaction,
      ledger::ResultCallback callback);

  void SendExternalTransaction(
      const ledger::Result result,
      const ledger::SKUTransaction& transaction,
      ledger::ResultCallback callback);

  void OnSendExternalTransaction(
      const int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      ledger::ResultCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_sku

#endif  // BRAVELEDGER_SKU_TRANSACTION_H_
