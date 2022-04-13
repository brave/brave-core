/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ASSET_RATIO_RESPONSE_PARSER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ASSET_RATIO_RESPONSE_PARSER_H_

#include <string>
#include <utility>
#include <vector>

#include "base/time/time.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

bool ParseAssetPrice(const std::string& json,
                     const std::vector<std::string>& from_assets,
                     const std::vector<std::string>& to_assets,
                     std::vector<mojom::AssetPricePtr>* values);
bool ParseAssetPriceHistory(const std::string& json,
                            std::vector<mojom::AssetTimePricePtr>* values);

std::string ParseEstimatedTime(const std::string& json);
mojom::GasEstimation1559Ptr ParseGasOracle(const std::string& json);
mojom::BlockchainTokenPtr ParseTokenInfo(const std::string& json,
                                         const std::string& chain_id,
                                         mojom::CoinType coin);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ASSET_RATIO_RESPONSE_PARSER_H_
