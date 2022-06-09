/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "build/buildflag.h"
#include "chrome/browser/sync/chrome_sync_client.h"
#include "components/sync/driver/sync_service.h"
#include "components/sync/driver/sync_user_settings.h"
#include "extensions/buildflags/buildflags.h"

#if !BUILDFLAG(ENABLE_EXTENSIONS)

#define CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_PROVIDER_H_
#define CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_SYNC_BRIDGE_H_

#endif  // !BUILDFLAG(ENABLE_EXTENSIONS)

// Default enabled type is Bookmarks.
#define OnLocalSyncTransportDataCleared                                     \
SetDefaultEnabledTypes(syncer::SyncService* sync_service) {                 \
  DCHECK(sync_service);                                                     \
                                                                            \
  syncer::UserSelectableTypeSet selected_types;                             \
  selected_types.Put(syncer::UserSelectableType::kBookmarks);               \
  sync_service->GetUserSettings()->SetSelectedTypes(false, selected_types); \
}                                                                           \
                                                                            \
void ChromeSyncClient::OnLocalSyncTransportDataCleared

#include "src/chrome/browser/sync/chrome_sync_client.cc"
#undef OnLocalSyncTransportDataCleared
