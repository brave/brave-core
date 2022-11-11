/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_WALLET_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_WALLET_UTIL_H_

#include <set>
#include <string>

#include "brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger_types.mojom.h"
#include "third_party/abseil-cpp/absl/types/variant.h"

namespace ledger {
class LedgerImpl;

namespace wallet {

mojom::ExternalWalletPtr GetWallet(LedgerImpl*, const std::string& wallet_type);

mojom::ExternalWalletPtr GetWalletIf(LedgerImpl*,
                                     const std::string& wallet_type,
                                     const std::set<mojom::WalletStatus>&);

bool SetWallet(LedgerImpl*, mojom::ExternalWalletPtr);

mojom::ExternalWalletPtr TransitionWallet(
    LedgerImpl*,
    absl::variant<mojom::ExternalWalletPtr, std::string> wallet_info,
    mojom::WalletStatus to);

mojom::ExternalWalletPtr MaybeCreateWallet(LedgerImpl*,
                                           const std::string& wallet_type);

bool LogOutWallet(LedgerImpl*,
                  const std::string& wallet_type,
                  const std::string& notification = "");

mojom::ExternalWalletPtr GenerateLinks(mojom::ExternalWalletPtr);

}  // namespace wallet
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_WALLET_UTIL_H_
