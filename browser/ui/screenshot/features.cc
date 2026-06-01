// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/screenshot/features.h"

namespace screenshot::features {

BASE_FEATURE(kBraveScreenshot,
             "BraveScreenshot",
             base::FEATURE_DISABLED_BY_DEFAULT);

bool IsScreenshotEnabled() {
  return base::FeatureList::IsEnabled(kBraveScreenshot);
}

}  // namespace screenshot::features
