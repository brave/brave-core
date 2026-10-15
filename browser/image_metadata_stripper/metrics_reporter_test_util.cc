/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/image_metadata_stripper/metrics_reporter_test_util.h"

#include "base/values.h"
#include "brave/browser/image_metadata_stripper/metrics_reporter.h"
#include "components/prefs/pref_service.h"

namespace image_metadata_stripper::metrics {

int GetFbmdStrippedCountInLocalState(PrefService* local_state,
                                     std::string_view flow_key) {
  const base::DictValue& root = local_state->GetDict(kRootPrefKey);
  const base::DictValue* flow = root.FindDict(flow_key);
  if (!flow) {
    return 0;
  }
  const base::DictValue* metadata = flow->FindDict(kFBMDMetadataPrefKey);
  if (!metadata) {
    return 0;
  }
  return metadata->FindInt(kStrippedPrefKey).value_or(0);
}

}  // namespace image_metadata_stripper::metrics
