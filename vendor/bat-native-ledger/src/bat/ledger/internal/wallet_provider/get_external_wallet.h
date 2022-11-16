/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_PROVIDER_GET_EXTERNAL_WALLET_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_PROVIDER_GET_EXTERNAL_WALLET_H_

#include <string>

#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace wallet_provider {

class GetExternalWallet {
 public:
  explicit GetExternalWallet(LedgerImpl*);

  virtual ~GetExternalWallet();

  void Run(ledger::GetExternalWalletCallback) const;

 protected:
  virtual const char* WalletType() const = 0;

  virtual void OnTransferTokens(ledger::GetExternalWalletCallback,
                                mojom::Result,
                                std::string drain_id) const;

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace wallet_provider
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_PROVIDER_GET_EXTERNAL_WALLET_H_
