/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/features.h"

#include "base/feature_list.h"
#include "brave/components/brave_wallet/common/buildflags.h"
#include "build/build_config.h"

namespace brave_wallet::features {

BASE_FEATURE(kNativeBraveWalletFeature,
             "NativeBraveWallet",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kBraveWalletBitcoinFeature,
             "BraveWalletBitcoin",
             base::FEATURE_ENABLED_BY_DEFAULT);
const base::FeatureParam<int> kBitcoinRpcThrottle{&kBraveWalletBitcoinFeature,
                                                  "rpc_throttle", 1};
const base::FeatureParam<bool> kBitcoinTestnetDiscovery{
    &kBraveWalletBitcoinFeature, "testnet_discovery", false};

BASE_FEATURE(kBraveWalletBitcoinImportFeature,
             "BraveWalletBitcoinImport",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kBraveWalletBitcoinLedgerFeature,
             "BraveWalletBitcoinLedger",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kBraveWalletZCashFeature,
             "BraveWalletZCash",
#if BUILDFLAG(ENABLE_ZCASH_BY_DEFAULT)
             base::FEATURE_ENABLED_BY_DEFAULT
#else
             base::FEATURE_DISABLED_BY_DEFAULT
#endif
);

const base::FeatureParam<bool> kZCashShieldedTransactionsEnabled{
    &kBraveWalletZCashFeature, "zcash_shielded_transactions_enabled", false};

BASE_FEATURE(kBraveWalletAnkrBalancesFeature,
             "BraveWalletAnkrBalances",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kBraveWalletTransactionSimulationsFeature,
             "BraveWalletTransactionSimulations",
             base::FEATURE_DISABLED_BY_DEFAULT);
}  // namespace brave_wallet::features
