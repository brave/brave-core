/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPHOLD_UPHOLD_WALLET_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPHOLD_UPHOLD_WALLET_H_

#include <string>

#include "bat/ledger/internal/endpoints/post_connect/post_connect.h"
#include "bat/ledger/internal/uphold/uphold_capabilities.h"
#include "bat/ledger/internal/uphold/uphold_user.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace uphold {

class UpholdWallet {
 public:
  explicit UpholdWallet(LedgerImpl*);

  ~UpholdWallet();

  void Generate(ledger::ResultCallback) const;

 private:
  void OnGetUser(ledger::ResultCallback, mojom::Result, const User&) const;

  void OnGetCapabilities(ledger::ResultCallback,
                         mojom::Result,
                         ledger::uphold::Capabilities) const;

  void OnCreateCard(ledger::ResultCallback,
                    mojom::Result,
                    std::string&& id) const;

  void OnConnectWallet(ledger::ResultCallback,
                       std::string&& id,
                       endpoints::PostConnect::Result&&) const;

  void OnTransferTokens(ledger::ResultCallback,
                        mojom::Result,
                        std::string drain_id) const;

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace uphold
}  // namespace ledger
#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPHOLD_UPHOLD_WALLET_H_
