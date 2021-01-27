/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_BITFLYER_BITFLYER_TRANSFER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_BITFLYER_BITFLYER_TRANSFER_H_

#include <map>
#include <memory>
#include <string>

#include "bat/ledger/internal/bitflyer/bitflyer.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace endpoint {
class BitflyerServer;
}

namespace bitflyer {

class BitflyerTransfer {
 public:
  explicit BitflyerTransfer(LedgerImpl* ledger);

  ~BitflyerTransfer();

  void Start(const Transaction& transaction,
             client::TransactionCallback callback);

 private:
  void OnCreateTransaction(const type::Result result,
                           const std::string& id,
                           client::TransactionCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<endpoint::BitflyerServer> bitflyer_server_;
};

}  // namespace bitflyer
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_BITFLYER_BITFLYER_TRANSFER_H_
