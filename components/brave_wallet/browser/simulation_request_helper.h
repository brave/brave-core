/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SIMULATION_REQUEST_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SIMULATION_REQUEST_HELPER_H_

#include <optional>
#include <string>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

namespace evm {

std::optional<std::string> EncodeScanTransactionParams(
    const mojom::TransactionInfo& tx_info);

}  // namespace evm

namespace solana {

std::optional<std::string> EncodeScanTransactionParams(
    const mojom::SignSolTransactionsRequest& sign_sol_transactions_request);
void PopulateRecentBlockhash(
    mojom::SignSolTransactionsRequest& sign_sol_transactions_request,
    const std::string& recent_blockhash);
bool HasEmptyRecentBlockhash(
    const mojom::SignSolTransactionsRequest& sign_sol_transactions_request);

std::optional<std::string> EncodeScanTransactionParams(
    const mojom::TransactionInfo& tx_info);
void PopulateRecentBlockhash(mojom::TransactionInfo& tx_info,
                             const std::string& recent_blockhash);
bool HasEmptyRecentBlockhash(const mojom::TransactionInfo& tx_info);

}  // namespace solana

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SIMULATION_REQUEST_HELPER_H_
