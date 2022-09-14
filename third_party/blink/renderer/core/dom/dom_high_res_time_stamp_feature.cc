/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/dom/dom_high_res_time_stamp_feature.h"

#include "base/feature_list.h"
#include "third_party/blink/public/common/features.h"

namespace brave {

bool IsTimeStampRoundingEnabled() {
  return base::FeatureList::IsEnabled(blink::features::kBraveRoundTimeStamps);
}

}  // namespace brave
