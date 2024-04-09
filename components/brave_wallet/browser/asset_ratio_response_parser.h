/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ASSET_RATIO_RESPONSE_PARSER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ASSET_RATIO_RESPONSE_PARSER_H_

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

std::optional<std::string> ParseSardineAuthToken(const base::Value& json_value);

bool ParseAssetPrice(const base::Value& json_value,
                     const std::vector<std::string>& from_assets,
                     const std::vector<std::string>& to_assets,
                     std::vector<mojom::AssetPricePtr>* values);
bool ParseAssetPriceHistory(const base::Value& json_value,
                            std::vector<mojom::AssetTimePricePtr>* values);
std::optional<std::vector<mojom::CoinMarketPtr>> ParseCoinMarkets(
    const base::Value& json_value);

std::optional<std::string> ParseStripeBuyURL(const base::Value& json_value);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ASSET_RATIO_RESPONSE_PARSER_H_
