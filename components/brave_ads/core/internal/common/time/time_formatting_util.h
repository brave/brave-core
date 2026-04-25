/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TIME_TIME_FORMATTING_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TIME_TIME_FORMATTING_UTIL_H_

#include <string>

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

// Returns a long human-readable date and time, e.g., "on Tuesday, 5 May 2020
// at 2:44:30 PM". If `use_sentence_style` is false, omits the leading "on ".
std::string LongFriendlyDateAndTime(base::Time time,
                                    bool use_sentence_style = true);

// Returns a human-readable date and time. If `time` is more than a day away,
// delegates to `LongFriendlyDateAndTime`. Otherwise returns a duration-style
// string, e.g., "in 0 hours, 21 minutes, 58 seconds at 15:07:30.568". If
// `use_sentence_style` is false, omits the leading "in ".
std::string FriendlyDateAndTime(base::Time time,
                                bool use_sentence_style = true);

// Formats a time compliant with ISO 8601 in UTC, e.g.,
// "2020-12-31T23:00:00.000Z", with fixed values for minutes, seconds,
// milliseconds, and time zone.
std::string TimeToPrivacyPreservingIso8601(base::Time time);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TIME_TIME_FORMATTING_UTIL_H_
