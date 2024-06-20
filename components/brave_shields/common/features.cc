/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/features.h"

namespace brave_shields {
namespace features {

BASE_FEATURE(kBraveShields,
             "BraveShields",
             base::FEATURE_ENABLED_BY_DEFAULT);

bool IsBraveShieldsEnabled() {
  return base::FeatureList::IsEnabled(features::kBraveShields);
}

}  // namespace features
}  // namespace brave_shields
