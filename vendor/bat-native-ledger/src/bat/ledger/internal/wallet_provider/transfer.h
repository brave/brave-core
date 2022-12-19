/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_PROVIDER_TRANSFER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_PROVIDER_TRANSFER_H_

#include <string>

#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace wallet_provider {

class Transfer {
 public:
  explicit Transfer(LedgerImpl*);

  virtual ~Transfer();

  void Run(const std::string& contribution_id,
           const std::string& destination,
           double amount,
           ledger::ResultCallback) const;

 protected:
  using MaybeCreateTransactionCallback =
      base::OnceCallback<void(std::string&&)>;

 private:
  void MaybeCreateTransaction(const std::string& contribution_id,
                              const std::string& destination,
                              double amount,
                              MaybeCreateTransactionCallback) const;

  void OnGetExternalTransactionId(MaybeCreateTransactionCallback,
                                  std::string&& contribution_id,
                                  std::string&& destination,
                                  double amount,
                                  std::string&& transaction_id) const;

  void SaveExternalTransaction(MaybeCreateTransactionCallback callback,
                               std::string&& contribution_id,
                               std::string&& destination,
                               double amount,
                               std::string&& transaction_id) const;

  void OnSaveExternalTransaction(MaybeCreateTransactionCallback,
                                 std::string&& transaction_id,
                                 mojom::Result) const;

 protected:
  virtual void CreateTransaction(MaybeCreateTransactionCallback,
                                 std::string&& destination,
                                 double amount) const;

  virtual void CommitTransaction(ledger::ResultCallback,
                                 std::string&& destination,
                                 double amount,
                                 std::string&& transaction_id) const = 0;

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace wallet_provider
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_PROVIDER_TRANSFER_H_
