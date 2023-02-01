// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/perf/brave_perf_switches.h"

namespace perf::switches {

// Enables some Brave's widely used features for a testing profile in perf
// tests. --user-data-dir should be set.
const char kEnableBraveFeaturesForPerfTesting[] =
    "enable-brave-features-for-perf-testing";

}  // namespace perf::switches
