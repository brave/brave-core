/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PERMISSION_RULES_ADS_PER_DAY_PERMISSION_RULE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PERMISSION_RULES_ADS_PER_DAY_PERMISSION_RULE_H_

#include <cstddef>
#include <vector>

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

bool HasAdsPerDayPermission(const std::vector<base::Time>& history, size_t cap);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PERMISSION_RULES_ADS_PER_DAY_PERMISSION_RULE_H_
