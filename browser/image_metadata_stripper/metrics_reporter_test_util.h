/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IMAGE_METADATA_STRIPPER_METRICS_REPORTER_TEST_UTIL_H_
#define BRAVE_BROWSER_IMAGE_METADATA_STRIPPER_METRICS_REPORTER_TEST_UTIL_H_

#include <string_view>

class PrefService;

namespace image_metadata_stripper::metrics {

// Reads the FBMD "stripped" count recorded in |local_state| for |flow_key|
// (kDownloadPrefKey or kUploadPrefKey). Returns 0 when nothing has been
// recorded for that flow yet.
int GetFbmdStrippedCountInLocalState(PrefService* local_state,
                                     std::string_view flow_key);

}  // namespace image_metadata_stripper::metrics

#endif  // BRAVE_BROWSER_IMAGE_METADATA_STRIPPER_METRICS_REPORTER_TEST_UTIL_H_
