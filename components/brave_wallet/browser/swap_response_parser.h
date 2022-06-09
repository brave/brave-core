/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SWAP_RESPONSE_PARSER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SWAP_RESPONSE_PARSER_H_

#include <string>
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

mojom::SwapResponsePtr ParseSwapResponse(const std::string& json,
                                         bool expect_transaction_data);

mojom::JupiterQuotePtr ParseJupiterQuote(const std::string& json);
mojom::JupiterSwapTransactionsPtr ParseJupiterSwapTransactions(
    const std::string& json);
absl::optional<std::string> ConvertJupiterQuoteUint64ToString(
    const std::string& json);
}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SWAP_RESPONSE_PARSER_H_
