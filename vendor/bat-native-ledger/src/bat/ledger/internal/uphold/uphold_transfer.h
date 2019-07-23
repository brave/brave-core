/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_UPHOLD_UPHOLD_TRANSFER_H_
#define BRAVELEDGER_UPHOLD_UPHOLD_TRANSFER_H_

#include <map>
#include <string>

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/uphold/uphold.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_uphold {

class UpholdTransfer {
 public:
  explicit UpholdTransfer(bat_ledger::LedgerImpl* ledger, Uphold* uphold);

  ~UpholdTransfer();

  void Start(double amount,
             const std::string& address,
             ledger::ExternalWalletPtr wallet,
             TransactionCallback callback);

 private:
  void CreateTransaction(double amount,
                         const std::string& address,
                         ledger::ExternalWalletPtr wallet,
                         TransactionCallback callback);

  void OnCreateTransaction(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const ledger::ExternalWallet& wallet,
    TransactionCallback callback);

  void CommitTransaction(const std::string& transaction_id,
                         const ledger::ExternalWallet& wallet,
                         TransactionCallback callback);

  void OnCommitTransaction(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    TransactionCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  Uphold* uphold_;  // NOT OWNED
};

}  // namespace braveledger_uphold
#endif  // BRAVELEDGER_UPHOLD_UPHOLD_TRANSFER_H_
