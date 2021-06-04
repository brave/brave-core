/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/sync/brave_profile_sync_service_delegate.h"
#include "brave/components/sync/driver/brave_sync_profile_sync_service.h"
#include "ios/chrome/browser/sync/device_info_sync_service_factory.h"

#define BRAVE_BUILD_SERVICE_INSTANCE_FOR                         \
  std::make_unique<syncer::BraveProfileSyncService>(             \
      std::move(init_params),                                    \
      std::make_unique<syncer::BraveProfileSyncServiceDelegate>( \
          DeviceInfoSyncServiceFactory::GetForBrowserState(browser_state)));

#include "../../../../../../ios/chrome/browser/sync/sync_service_factory.cc"

#undef BRAVE_BUILD_SERVICE_INSTANCE_FOR
