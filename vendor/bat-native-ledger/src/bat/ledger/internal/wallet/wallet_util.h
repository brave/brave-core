/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_WALLET_WALLET_UTIL_H_
#define BRAVELEDGER_WALLET_WALLET_UTIL_H_

#include <map>
#include <string>

#include "bat/ledger/ledger.h"

namespace ledger {
namespace wallet {

ledger::ExternalWalletPtr GetWallet(
    const std::string& wallet_type,
    std::map<std::string, ledger::ExternalWalletPtr> wallets);

ledger::ExternalWalletPtr ResetWallet(ledger::ExternalWalletPtr wallet);

}  // namespace wallet
}  // namespace ledger

#endif  // BRAVELEDGER_WALLET_WALLET_UTIL_H_
