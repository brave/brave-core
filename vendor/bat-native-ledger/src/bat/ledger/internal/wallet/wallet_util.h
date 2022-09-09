/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_WALLET_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_WALLET_UTIL_H_

#include <map>
#include <string>
#include <vector>

#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace wallet {

mojom::ExternalWalletPtr ExternalWalletPtrFromJSON(std::string wallet_string,
                                                   std::string wallet_type);

mojom::ExternalWalletPtr GetWallet(LedgerImpl* ledger,
                                   const std::string wallet_type);

bool SetWallet(LedgerImpl* ledger,
               mojom::ExternalWalletPtr wallet,
               const std::string wallet_type);

mojom::ExternalWalletPtr ResetWallet(mojom::ExternalWalletPtr wallet);

void OnWalletStatusChange(LedgerImpl* ledger,
                          absl::optional<mojom::WalletStatus> from,
                          mojom::WalletStatus to);

}  // namespace wallet
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_WALLET_UTIL_H_
