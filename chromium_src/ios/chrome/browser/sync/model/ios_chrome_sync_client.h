/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_SYNC_MODEL_IOS_CHROME_SYNC_CLIENT_H_
#define BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_SYNC_MODEL_IOS_CHROME_SYNC_CLIENT_H_

class IOSChromeSyncClient;
using IOSChromeSyncClient_BraveImpl = IOSChromeSyncClient;

#define IOSChromeSyncClient IOSChromeSyncClient_ChromiumImpl
#define component_factory_ \
  component_factory_;      \
  friend IOSChromeSyncClient_BraveImpl

#include "src/ios/chrome/browser/sync/model/ios_chrome_sync_client.h"  // IWYU pragma: export
#undef component_factory_
#undef IOSChromeSyncClient

class IOSChromeSyncClient : public IOSChromeSyncClient_ChromiumImpl {
 public:
  using IOSChromeSyncClient_ChromiumImpl::IOSChromeSyncClient_ChromiumImpl;

  IOSChromeSyncClient(const IOSChromeSyncClient&) = delete;
  IOSChromeSyncClient& operator=(const IOSChromeSyncClient&) = delete;

  ~IOSChromeSyncClient() override;

  // BrowserSyncClient implementation.
  syncer::DataTypeController::TypeVector CreateDataTypeControllers(
      syncer::SyncService* sync_service) override;
};

#endif  // BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_SYNC_MODEL_IOS_CHROME_SYNC_CLIENT_H_
