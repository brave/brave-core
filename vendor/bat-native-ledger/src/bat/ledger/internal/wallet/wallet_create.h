/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_WALLET_CREATE_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_WALLET_CREATE_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "bat/ledger/internal/endpoint/promotion/promotion_server.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace wallet {

class WalletCreate {
 public:
  explicit WalletCreate(LedgerImpl* ledger);
  ~WalletCreate();

  void Start(ledger::ResultCallback callback);

 private:
  void OnCreate(ledger::ResultCallback callback,
                mojom::Result result,
                const std::string& payment_id);

  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<endpoint::PromotionServer> promotion_server_;
};

}  // namespace wallet
}  // namespace ledger
#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_WALLET_CREATE_H_
