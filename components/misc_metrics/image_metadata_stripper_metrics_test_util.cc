/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/image_metadata_stripper_metrics_test_util.h"

#include "base/values.h"
#include "brave/components/misc_metrics/image_metadata_stripper_metrics.h"
#include "components/prefs/pref_service.h"

namespace image_metadata_stripper::metrics {

int GetFbmdStrippedCountInLocalState(PrefService* local_state,
                                     std::string_view count_key) {
  const base::DictValue& root = local_state->GetDict(kRootPrefKey);
  const base::DictValue* fbmd = root.FindDict(kFbmdPrefKey);
  if (!fbmd) {
    return 0;
  }
  return fbmd->FindInt(count_key).value_or(0);
}

}  // namespace image_metadata_stripper::metrics
