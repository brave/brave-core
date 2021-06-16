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

namespace brave_wallet {

using AssetTimePrice = std::pair<base::Time, std::string>;

bool ParseAssetPrice(const std::string& json, std::string* price);
bool ParseAssetPriceHistory(const std::string& json,
                            std::vector<AssetTimePrice>* values);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ASSET_RATIO_RESPONSE_PARSER_H_
