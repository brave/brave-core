// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_SWITCHES_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_SWITCHES_H_

namespace brave_wallet::switches {

// Allows auto unlocking wallet password with command line.
inline constexpr char kDevWalletPassword[] = "dev-wallet-password";

// Ratios service dev URL
inline constexpr char kAssetRatioDevUrl[] = "asset-ratio-dev-url";
inline constexpr char kMeldAssetRatioDevUrl[] = "meld-asset-ratio-dev-url";

// ZCash mainnet rpc endpoint.
inline constexpr char kZCashMainnetRpcUrl[] = "zcash-mainnet-rpc-url";

// ZCash testnet rpc endpoint.
inline constexpr char kZCashTestnetRpcUrl[] = "zcash-testnet-rpc-url";

// Cardano mainnet rpc endpoint.
inline constexpr char kCardanoMainnetRpcUrl[] = "cardano-mainnet-rpc-url";

// Cardano testnet rpc endpoint.
inline constexpr char kCardanoTestnetRpcUrl[] = "cardano-testnet-rpc-url";

// Polkadot mainnet rpc endpoint.
inline constexpr char kPolkadotMainnetRpcUrl[] = "polkadot-mainnet-rpc-url";

// Polkadot testnet rpc endpoint.
inline constexpr char kPolkadotTestnetRpcUrl[] = "polkadot-testnet-rpc-url";

// Cardano mainnet rpc project id.
inline constexpr char kCardanoMainnetProjectId[] = "cardano-mainnet-project-id";

// Cardano testnet rpc project id.
inline constexpr char kCardanoTestnetProjectId[] = "cardano-testnet-project-id";

}  // namespace brave_wallet::switches

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_SWITCHES_H_
