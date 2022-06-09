/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SYNC_CHROME_SYNC_CLIENT_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SYNC_CHROME_SYNC_CLIENT_H_

#include "components/sync/driver/sync_client.h"

#define profile profile, bool is_rewards_chrome_sync_client = false

#define OnLocalSyncTransportDataCleared                             \
SetDefaultEnabledTypes(syncer::SyncService* sync_service) override; \
void OnLocalSyncTransportDataCleared

#include "src/chrome/browser/sync/chrome_sync_client.h"
#undef OnLocalSyncTransportDataCleared
#undef profile

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SYNC_CHROME_SYNC_CLIENT_H_
