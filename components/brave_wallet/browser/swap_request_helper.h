/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SWAP_REQUEST_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SWAP_REQUEST_HELPER_H_

#include <optional>
#include <string>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

namespace jupiter {
std::optional<std::string> EncodeTransactionParams(
    const mojom::JupiterTransactionParams& params);
}  // namespace jupiter

namespace lifi {
std::optional<std::string> EncodeQuoteParams(
    mojom::SwapQuoteParamsPtr params,
    const std::optional<std::string>& fee_param);

std::optional<std::string> EncodeTransactionParams(mojom::LiFiStepPtr step);
}  // namespace lifi

namespace squid {
std::optional<std::string> EncodeQuoteParams(mojom::SwapQuoteParamsPtr params);
std::optional<std::string> EncodeTransactionParams(
    mojom::SwapQuoteParamsPtr params);
}  // namespace squid

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SWAP_REQUEST_HELPER_H_
