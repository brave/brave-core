// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_screenshots/common/features.h"

namespace brave_screenshots::features {

BASE_FEATURE(kBraveScreenshots,
             "BraveScreenshots",
             base::FEATURE_ENABLED_BY_DEFAULT);

}  // namespace brave_screenshots::features
