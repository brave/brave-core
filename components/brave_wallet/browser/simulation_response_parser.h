/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SIMULATION_RESPONSE_PARSER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SIMULATION_RESPONSE_PARSER_H_

#include <optional>
#include <string>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

namespace evm {

mojom::EVMSimulationResponsePtr ParseSimulationResponse(
    const base::Value& json_value);

}  // namespace evm

namespace solana {

mojom::SolanaSimulationResponsePtr ParseSimulationResponse(
    const base::Value& json_value);

}  // namespace solana

std::optional<std::string> ParseSimulationErrorResponse(
    const base::Value& json_value);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SIMULATION_RESPONSE_PARSER_H_
