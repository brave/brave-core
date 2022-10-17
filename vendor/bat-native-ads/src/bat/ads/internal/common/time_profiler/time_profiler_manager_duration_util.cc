/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/common/time_profiler/time_profiler_manager_duration_util.h"

#include "base/check.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "bat/ads/internal/common/time_profiler/pretty_function_parser_util.h"
#include "bat/ads/internal/common/time_profiler/time_profile_info.h"

namespace ads {

namespace {

std::string BuildLocation(const std::string& category_group,
                          const TimeProfileInfo& time_profile) {
  DCHECK(!category_group.empty());
  DCHECK(!time_profile.name.empty());

  return base::StrCat({category_group, "|", time_profile.name});
}

}  // namespace

std::string GetDurationSinceLastTimeTicks(const base::TimeTicks& time_ticks) {
  const base::TimeDelta duration = base::TimeTicks::Now() - time_ticks;
  return base::StrCat(
      {base::NumberToString(duration.InMillisecondsF()), " ms"});
}

std::string BuildDurationSinceLastTimeTicksLogMessage(
    const std::string& category_group,
    const int line,
    const std::string& message,
    const TimeProfileInfo& time_profile) {
  DCHECK(!category_group.empty());

  const std::string location = BuildLocation(category_group, time_profile);

  const std::string duration =
      GetDurationSinceLastTimeTicks(time_profile.last_time_ticks);

  return base::StrCat({"TimeProfiler.Duration [", location, ".",
                       base::NumberToString(line), "] ", message, ": ",
                       duration});
}

}  // namespace ads
