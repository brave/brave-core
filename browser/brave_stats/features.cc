/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_stats/features.h"

namespace brave_stats {
namespace features {

// Report a special refcode (`HED001`) if a headless or automation
// modes are enabled. If feature is disabled, the usage ping will not
// be sent for these clients.
BASE_FEATURE(kHeadlessRefcode,
             "BraveStatsHeadlessRefcode",
             base::FEATURE_ENABLED_BY_DEFAULT);

bool IsHeadlessClientRefcodeEnabled() {
  return base::FeatureList::IsEnabled(features::kHeadlessRefcode);
}

}  // namespace features
}  // namespace brave_stats
