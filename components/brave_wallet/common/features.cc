/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/features.h"

#include "base/feature_list.h"
#include "build/build_config.h"

namespace brave_wallet {
namespace features {

const base::Feature kNativeBraveWalletFeature{"NativeBraveWallet",
                                              base::FEATURE_ENABLED_BY_DEFAULT};
const base::Feature kBraveWalletFilecoinFeature{
    "BraveWalletFilecoin", base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kBraveWalletSolanaFeature{
#if BUILDFLAG(IS_ANDROID)
  "BraveWalletSolana", base::FEATURE_DISABLED_BY_DEFAULT
#else
  "BraveWalletSolana", base::FEATURE_ENABLED_BY_DEFAULT
#endif
};

const base::Feature kBraveWalletSolanaProviderFeature{
    "BraveWalletSolanaProvider", base::FEATURE_DISABLED_BY_DEFAULT};

}  // namespace features
}  // namespace brave_wallet
