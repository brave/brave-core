/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_CONSTANTS_H_

#include <vector>

#include "base/no_destructor.h"
#include "brave/components/brave_wallet/browser/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

extern const char kAssetRatioBaseURL[];
extern const char kSwapBaseURL[];

constexpr uint256_t kDefaultSendEthGasLimit = 21000;
constexpr uint256_t kDefaultSendEthGasPrice = 150000000000ULL;  // 150 Gwei

// List of assets from Wyre, available to buy
static base::NoDestructor<std::vector<mojom::ERCToken>> kBuyTokens(
    {{"", "Basic Attention Token", "bat.svg", true, false, "BAT", 0, true},
     {"", "Ethereum", "", false, false, "ETH", 0, true},
     {"", "USD Coin", "usdc.svg", true, false, "USDC", 0, true},
     {"", "DAI", "dai.svg", true, false, "DAI", 0, true},
     {"", "AAVE", "AAVE.svg", true, false, "AAVE", 0, true},
     {"", "Binance USD", "busd.svg", true, false, "BUSD", 0, true},
     {"", "Compound", "comp.svg", true, false, "Comp", 0, true},
     {"", "Curve", "", true, false, "CRV", 0, true},
     {"", "Gemini Dollar", "gusd.svg", true, false, "GUSD", 0, true},
     {"", "Chainlink", "chainlink.svg", true, false, "LINK", 0, true},
     {"", "Maker", "mkr.svg", true, false, "MKR", 0, true},
     {"", "Paxos Standard", "pax.svg", true, false, "PAX", 0, true},
     {"", "Synthetix", "synthetix.svg", true, false, "SNX", 0, true},
     {"", "UMA", "UMA.png", true, false, "UMA", 0, true},
     {"", "Uniswap", "uni.svg", true, false, "UNI", 0, true},
     {"", "Stably Dollar", "usds.svg", true, false, "USDS", 0, true},
     {"", "Tether", "usdt.svg", true, false, "USDT", 0, true},
     {"", "Wrapped Bitcoin", "wbtc.svg", true, false, "WBTC", 0, true},
     {"", "Year.Finance", "yfi.svg", true, false, "YFI", 0, true},
     {"", "Palm DAI", "", true, false, "PDAI", 0, true},
     {"", "Matic USDC", "", true, false, "MUSDC", 0, true}});

const char kWalletBaseDirectory[] = "BraveWallet";
const char kImageSourceHost[] = "erc-token-images";

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_CONSTANTS_H_
