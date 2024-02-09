/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_DAYPART_EXCLUSION_RULE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_DAYPART_EXCLUSION_RULE_UTIL_H_

namespace brave_ads {

struct CreativeDaypartInfo;

bool MatchDayOfWeek(const CreativeDaypartInfo& daypart, int day_of_week);

bool MatchTimeSlot(const CreativeDaypartInfo& daypart, int minutes);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_DAYPART_EXCLUSION_RULE_UTIL_H_
