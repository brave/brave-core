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

namespace squid {

mojom::SquidQuotePtr ParseQuoteResponse(const base::Value& json_value);
mojom::SquidErrorPtr ParseErrorResponse(const base::Value& json_value);
mojom::SquidTransactionUnionPtr ParseTransactionResponse(
    const base::Value& json_value);

}  // namespace squid

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SWAP_RESPONSE_PARSER_H_
