/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_REQUESTS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_REQUESTS_H_

#include <string>
#include <vector>

namespace brave_wallet {

namespace solana {

std::string getBalance(const std::string& pubkey);
std::string getTokenAccountBalance(const std::string& pubkey);
std::string sendTransaction(const std::string& signed_tx);
std::string getLatestBlockhash();
std::string getSignatureStatuses(const std::vector<std::string>& tx_signatures);

}  // namespace solana

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_REQUESTS_H_
