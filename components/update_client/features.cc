/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/update_client/features.h"

#include "build/build_config.h"

static_assert(BUILDFLAG(IS_MAC), "Currently for macOS only");

namespace brave {

// DO NOT TURN THIS FEATURE ON IN PRODUCTION. As of this writing, it only
// implements the happy path of switching from Sparkle to Omaha 4 for per-user
// installations. It does not handle system-wide implementations, nor switching
// from Omaha 4 back to Sparkle. When you do enable the feature in the future,
// make sure that there are no old clients that suffer from the above
// limitations. In practice, this will very likely mean that you will need to
// rename the feature.
BASE_FEATURE(kBraveUseOmaha4,
             "BraveUseOmaha4",
             base::FEATURE_DISABLED_BY_DEFAULT);

bool ShouldUseOmaha4() {
  return base::FeatureList::IsEnabled(kBraveUseOmaha4);
}

}  // namespace brave
