// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/commerce/features.h"

namespace brave::features {

BASE_FEATURE(kBraveAllowExternalPurchaseLinks,
             "BraveAllowExternalPurchaseLinks",
             base::FEATURE_ENABLED_BY_DEFAULT);
}
