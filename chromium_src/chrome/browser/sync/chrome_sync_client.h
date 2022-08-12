/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SYNC_CHROME_SYNC_CLIENT_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SYNC_CHROME_SYNC_CLIENT_H_

// Forward include to avoid redefine CreateDataTypeControllers at
// BrowserSyncClient
#include "components/browser_sync/browser_sync_client.h"

#define CreateDataTypeControllers                                            \
  CreateDataTypeControllers_ChromiumImpl(syncer::SyncService* sync_service); \
  syncer::DataTypeController::TypeVector CreateDataTypeControllers

#include "src/chrome/browser/sync/chrome_sync_client.h"

#undef CreateDataTypeControllers

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SYNC_CHROME_SYNC_CLIENT_H_
