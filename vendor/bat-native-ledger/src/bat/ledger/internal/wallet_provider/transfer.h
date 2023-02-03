/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_PROVIDER_TRANSFER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_PROVIDER_TRANSFER_H_

#include <string>

#include "base/types/expected.h"
#include "bat/ledger/internal/database/database_external_transactions.h"
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
      base::OnceCallback<void(mojom::ExternalTransactionPtr)>;

 private:
  void MaybeCreateTransaction(const std::string& contribution_id,
                              const std::string& destination,
                              const std::string& amount,
                              MaybeCreateTransactionCallback) const;

  void OnGetExternalTransaction(
      MaybeCreateTransactionCallback,
      std::string&& contribution_id,
      std::string&& destination,
      std::string&& amount,
      base::expected<mojom::ExternalTransactionPtr,
                     database::GetExternalTransactionError>) const;

  void SaveExternalTransaction(MaybeCreateTransactionCallback callback,
                               mojom::ExternalTransactionPtr) const;

  void OnSaveExternalTransaction(MaybeCreateTransactionCallback,
                                 mojom::ExternalTransactionPtr,
                                 mojom::Result) const;

 protected:
  virtual void CreateTransaction(MaybeCreateTransactionCallback,
                                 mojom::ExternalTransactionPtr) const;

  virtual void CommitTransaction(ledger::ResultCallback,
                                 mojom::ExternalTransactionPtr) const = 0;

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace wallet_provider
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_PROVIDER_TRANSFER_H_
