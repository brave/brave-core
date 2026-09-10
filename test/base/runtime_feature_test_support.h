/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_TEST_BASE_RUNTIME_FEATURE_TEST_SUPPORT_H_
#define BRAVE_TEST_BASE_RUNTIME_FEATURE_TEST_SUPPORT_H_

#include <vector>

#include "third_party/blink/public/platform/web_runtime_features.h"

namespace brave {

// Snapshots Blink's feature values after Brave and Content initialize renderer
// features, then restores the calling test's runtime state and command line.
std::vector<blink::WebRuntimeFeatures::RuntimeFeatureStateForTesting>
GetRendererRuntimeFeatureStatesForTesting();

}  // namespace brave

#endif  // BRAVE_TEST_BASE_RUNTIME_FEATURE_TEST_SUPPORT_H_
