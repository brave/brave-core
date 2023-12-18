/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SWAP_RESPONSE_PARSER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SWAP_RESPONSE_PARSER_H_

#include <optional>
#include <string>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

mojom::ZeroExQuotePtr ParseZeroExQuoteResponse(const base::Value& json_value,
                                               bool expect_transaction_data);
mojom::ZeroExErrorPtr ParseZeroExErrorResponse(const base::Value& json_value);

mojom::JupiterQuotePtr ParseJupiterQuoteResponse(const base::Value& json_value);
std::optional<std::string> ParseJupiterTransactionResponse(
    const base::Value& json_value);
mojom::JupiterErrorPtr ParseJupiterErrorResponse(const base::Value& json_value);
std::optional<std::string> ConvertAllNumbersToString(const std::string& json);
}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SWAP_RESPONSE_PARSER_H_
