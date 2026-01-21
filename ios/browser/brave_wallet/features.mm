// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_wallet/features.h"

#include "base/feature_list.h"
#include "brave/components/brave_wallet/common/features.h"

namespace brave_wallet {
namespace features {

BASE_FEATURE(kBraveWalletWebUIIOS,
             base::FEATURE_DISABLED_BY_DEFAULT);

bool IsWalletDebugEnabled() {
#if !defined(OFFICIAL_BUILD)
  return base::FeatureList::IsEnabled(features::kBraveWalletDebugFeature);
#else
  return false;
#endif
}
}
}  // namespace brave_wallet
