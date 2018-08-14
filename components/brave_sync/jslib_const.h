/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_JSLIB_CONST_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_JSLIB_CONST_H_

namespace brave_sync {

namespace jslib_const {

extern const char kBookmarks[];
extern const char kHistorySites[];
extern const char kPreferences[];

const int kActionCreate = 0;
const int kActionUpdate = 1;
const int kActionDelete = 2;

extern const char DEVICES_NAMES[];

extern const char CREATE_RECORD[];
extern const char UPDATE_RECORD[];
extern const char DELETE_RECORD[];

extern const char SyncObjectData_BOOKMARK[];
extern const char SyncObjectData_HISTORY_SITE[];
extern const char SyncObjectData_SITE_SETTING[];
extern const char SyncObjectData_DEVICE[];

extern const char SyncRecordType_BOOKMARKS[];
extern const char SyncRecordType_HISTORY[];
extern const char SyncRecordType_PREFERENCES[];

} // jslib_const

} // namespace brave_sync

#endif //BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_JSLIB_CONST_H_
