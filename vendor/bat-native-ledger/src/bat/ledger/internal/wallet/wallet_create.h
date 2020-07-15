/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_WALLET_WALLET_CREATE_H_
#define BRAVELEDGER_WALLET_WALLET_CREATE_H_

#include <stdint.h>

#include <string>

#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_wallet {

class WalletCreate {
 public:
  explicit WalletCreate(bat_ledger::LedgerImpl* ledger);
  ~WalletCreate();

  void Start(ledger::ResultCallback callback);

 private:
  void OnCreate(
      const ledger::UrlResponse& response,
      ledger::ResultCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_wallet
#endif  // BRAVELEDGER_WALLET_WALLET_CREATE_H_
