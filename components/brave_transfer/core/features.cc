// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_transfer/core/features.h"

namespace brave_transfer::features {

BASE_FEATURE(kBraveTransfer, base::FEATURE_DISABLED_BY_DEFAULT);

bool IsBraveTransferEnabled() {
  return base::FeatureList::IsEnabled(kBraveTransfer);
}

}  // namespace brave_transfer::features
