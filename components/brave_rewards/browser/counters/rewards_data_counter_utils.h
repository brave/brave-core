// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_REWARDS_DATA_COUNTERS_REWARDS_DATA_COUNTER_UTILS_H_
#define BRAVE_BROWSER_REWARDS_DATA_COUNTERS_REWARDS_DATA_COUNTER_UTILS_H_

#include "base/strings/string16.h"
#include "components/browsing_data/core/counters/browsing_data_counter.h"

class Profile;

namespace rewards_data_counter_utils {

// Constructs the text to be displayed by a counter from the given |result|.
base::string16 GetBraveCounterTextFromResult(
    const browsing_data::BrowsingDataCounter::Result* result,
    Profile* profile);

base::string16 GetAutoContributeCounterText(
    const std::string& pref_name,
    const browsing_data::BrowsingDataCounter::Result* result);

// Constructs the text to be displayed by a counter from the given |result|.
// Currently this can only be used for counters for which the Result is
// defined in components/browsing_data/core/counters.
base::string16 GetCounterTextFromResult(
    const browsing_data::BrowsingDataCounter::Result* result);

}  // namespace rewards_data_counter_utils

#endif  // BRAVE_BROWSER_REWARDS_DATA_COUNTERS_REWARDS_DATA_COUNTER_UTILS_H_
