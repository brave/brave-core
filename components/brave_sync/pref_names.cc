/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/pref_names.h"

namespace brave_sync {

namespace prefs {


// String of device id. Supposed to be an integer
const char kThisDeviceId[] = "bravesync.device_id";

// String of 32 comma separated bytes
// like "145,58,125,111,85,164,236,38,204,67,40,31,182,114,14,152,242,26,197,245,235,130,72,251,116,18,3,222,245,245,61,114"
const char kSeed[] = "bravesync.seed";

// String of current device namefor sync
const char kThisDeviceName[] = "bravesync.device_name";

// Boolean, whether sync is enabled for the current device
// If true, then sync is enabled and running
// If false, then sync is not enabled or not running (disabled after enabling,
// but seed and device id are configured) //TODO, AB: maybe split these
// to separate entries
const char kSyncThisDeviceEnabled[] = "bravesync.sync_this_device_enabled";

const char kSyncBookmarksEnabled[] = "bravesync.bookmarks_enabled";
const char kSiteSettingsEnabled[] = "bravesync.site_settings_enabled";
const char kHistoryEnabled[] = "bravesync.history_enabled";

// The latest time of synced record, field 'syncTimestamp'
const char kLatestRecordTime[] = "bravesync.latest_record_time";
// The time of latest fetch records operation
const char kLastFetchTime[] = "bravesync.last_fetch_time";


} // namespace prefs

} // namespace brave_sync
