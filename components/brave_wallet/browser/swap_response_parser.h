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

namespace zeroex {

mojom::ZeroExQuotePtr ParseQuoteResponse(const base::Value& json_value,
                                         const std::string& chain_id);
mojom::ZeroExTransactionPtr ParseTransactionResponse(
    const base::Value& json_value);
mojom::ZeroExErrorPtr ParseErrorResponse(const base::Value& json_value);

}  // namespace zeroex

namespace jupiter {

mojom::JupiterQuotePtr ParseQuoteResponse(const base::Value& json_value);
std::optional<std::string> ParseTransactionResponse(
    const base::Value& json_value);
mojom::JupiterErrorPtr ParseErrorResponse(const base::Value& json_value);

}  // namespace jupiter

namespace lifi {

mojom::LiFiTransactionUnionPtr ParseTransactionResponse(
    const base::Value& json_value);
mojom::LiFiQuotePtr ParseQuoteResponse(const base::Value& json_value);
mojom::LiFiErrorPtr ParseErrorResponse(const base::Value& json_value);
mojom::LiFiStatusPtr ParseStatusResponse(const base::Value& json_value);

}  // namespace lifi

namespace gate3 {

// Parse a Gate3 quote response JSON into a structured Gate3SwapQuote object.
// The response contains one or more swap routes, sorted by the best route
// first. Return nullptr if parsing fails, or if the response contains no valid
// routes.
//
// See swap_responses.idl and https://gate3.bsg.brave.com/docs for the
// underlying response format.
mojom::Gate3SwapQuotePtr ParseQuoteResponse(const base::Value& json_value);

// Parse a Gate3 error response JSON into a structured Gate3SwapError object.
// Return nullptr if parsing fails.
//
// See swap_responses.idl and https://gate3.bsg.brave.com/docs for the
// underlying response format.
mojom::Gate3SwapErrorPtr ParseErrorResponse(const base::Value& json_value);

// Parse a Gate3 status response JSON into a structured Gate3SwapStatus object.
// Return nullptr if parsing fails.
//
// See swap_responses.idl and https://gate3.bsg.brave.com/docs for the
// underlying response format.
mojom::Gate3SwapStatusPtr ParseStatusResponse(const base::Value& json_value);

}  // namespace gate3

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SWAP_RESPONSE_PARSER_H_
