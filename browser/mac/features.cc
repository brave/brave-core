/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/mac/features.h"

#include "brave/browser/sparkle_buildflags.h"

namespace {

BASE_FEATURE(kBraveUseOmaha4,
             "BraveUseOmaha4",
             base::FEATURE_DISABLED_BY_DEFAULT);

}

bool ShouldUseSparkle() {
#if BUILDFLAG(ENABLE_SPARKLE)
  return !ShouldUseOmaha4();
#else
  return false;
#endif
}

bool ShouldUseOmaha4() {
  return base::FeatureList::IsEnabled(kBraveUseOmaha4);
}
