/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/features.h"

#include "base/feature_list.h"
#include "build/build_config.h"

namespace brave_wallet {
namespace features {

BASE_FEATURE(kNativeBraveWalletFeature,
             "NativeBraveWallet",
             base::FEATURE_ENABLED_BY_DEFAULT);
const base::FeatureParam<bool> kShowToolbarTxStatus{
    &kNativeBraveWalletFeature, "show_toolbar_tx_status", true};

BASE_FEATURE(kBraveWalletFilecoinFeature,
             "BraveWalletFilecoin",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kBraveWalletSolanaFeature,
             "BraveWalletSolana",
             base::FEATURE_ENABLED_BY_DEFAULT);
const base::FeatureParam<bool> kCreateDefaultSolanaAccount{
    &kBraveWalletSolanaFeature, "create_default_solana_account", true};

BASE_FEATURE(kBraveWalletSolanaProviderFeature,
             "BraveWalletSolanaProvider",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kBraveWalletDappsSupportFeature,
             "BraveWalletDappsSupport",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kBraveWalletENSL2Feature,
             "BraveWalletENSL2",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kBraveWalletSnsFeature,
             "BraveWalletSns",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kBraveWalletNftPinningFeature,
             "BraveWalletNftPinning",
#if BUILDFLAG(IS_ANDROID)
             base::FEATURE_DISABLED_BY_DEFAULT
#else
             base::FEATURE_ENABLED_BY_DEFAULT
#endif
);

BASE_FEATURE(kBraveWalletPanelV2Feature,
             "BraveWalletPanelV2",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kBraveWalletBitcoinFeature,
             "BraveWalletBitcoin",
             base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace features
}  // namespace brave_wallet
