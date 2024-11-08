/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_ENTRIES_LAST_UNIDLE_TIME_DIAGNOSTIC_ENTRY_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_ENTRIES_LAST_UNIDLE_TIME_DIAGNOSTIC_ENTRY_UTIL_H_

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

void SetLastUnIdleTimeDiagnosticEntry(base::Time last_unidle_at);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_ENTRIES_LAST_UNIDLE_TIME_DIAGNOSTIC_ENTRY_UTIL_H_
