/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/brave_origin_utils.h"

#include "base/feature_list.h"
#include "brave/components/brave_origin/features.h"

namespace brave_origin {

bool IsBraveOriginEnabled() {
  // TODO(https://github.com/brave/brave-browser/issues/47463)
  // Get the actual purchase state from SKU service.
#if DCHECK_IS_ON()  // Debug builds only
  return base::FeatureList::IsEnabled(features::kBraveOrigin);
#else
  return false;  // Always disabled in release builds
#endif
}

}  // namespace brave_origin