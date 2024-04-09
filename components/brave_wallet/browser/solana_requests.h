/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_REQUESTS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_REQUESTS_H_

#include <optional>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/solana_transaction.h"

namespace brave_wallet {

namespace solana {

std::string getBalance(const std::string& pubkey);
std::string getTokenAccountBalance(const std::string& pubkey);
std::string sendTransaction(
    const std::string& signed_tx,
    std::optional<SolanaTransaction::SendOptions> options);
std::string getLatestBlockhash();
std::string getSignatureStatuses(const std::vector<std::string>& tx_signatures);
std::string getAccountInfo(const std::string& pubkey);
std::string getFeeForMessage(const std::string& message);
std::string getBlockHeight();
std::string getTokenAccountsByOwner(const std::string& pubkey,
                                    const std::string& encoding);
std::string isBlockhashValid(const std::string& blockhash,
                             const std::optional<std::string>& commitment);
std::string simulateTransaction(const std::string& unsigned_tx);
std::string getRecentPrioritizationFees();

}  // namespace solana

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_REQUESTS_H_
