/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef BRAVE_COMPONENT_BRAVE_SYNC_PREF_NAMES_H_
#define BRAVE_COMPONENT_BRAVE_SYNC_PREF_NAMES_H_

namespace brave_sync {

namespace prefs {

extern const char kThisDeviceId[];
extern const char kSeed[];
extern const char kThisDeviceName[];
extern const char kBookmarksBaseOrder[];

extern const char kSyncThisDeviceEnabled[];
extern const char kSyncBookmarksEnabled[];
extern const char kSiteSettingsEnabled[];
extern const char kHistoryEnabled[];

extern const char kLatestRecordTime[];
extern const char kLastFetchTime[];

} // namespace prefs

} // namespace brave_sync

#endif //BRAVE_COMPONENT_BRAVE_SYNC_PREF_NAMES_H_
