// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_STATS_SWITCHES_H_
#define BRAVE_BROWSER_BRAVE_STATS_SWITCHES_H_

namespace brave_stats {

namespace switches {

// Allows setting the usage server to a custom host. Useful both for manual
// testing against staging and for browser tests.
constexpr char kBraveStatsUpdaterServer[] =
    "brave-stats-updater-server";

}  // namespace switches
}  // namespace brave_stats

#endif  // BRAVE_BROWSER_BRAVE_STATS_SWITCHES_H_
