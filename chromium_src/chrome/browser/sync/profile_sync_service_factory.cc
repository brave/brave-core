/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/signin/brave_identity_manager_factory.h"
#include "brave/browser/sync/brave_profile_sync_service_delegate.h"
#include "brave/components/sync/driver/brave_sync_profile_sync_service.h"
#include "chrome/browser/sync/device_info_sync_service_factory.h"

#define IdentityManagerFactory BraveIdentityManagerFactory

#define BRAVE_BUILD_SERVICE_INSTANCE_FOR                         \
  std::make_unique<syncer::BraveProfileSyncService>(             \
      std::move(init_params),                                    \
      std::make_unique<syncer::BraveProfileSyncServiceDelegate>( \
          DeviceInfoSyncServiceFactory::GetForProfile(profile)));

// Excluding src/chrome/browser/signin/identity_manager_factory.h, but this is
// OK, because we are including brave_identity_manager_factory.h
#define CHROME_BROWSER_SIGNIN_IDENTITY_MANAGER_FACTORY_H_

#include "../../../../../chrome/browser/sync/profile_sync_service_factory.cc"

#undef BRAVE_BUILD_SERVICE_INSTANCE_FOR

#undef IdentityManagerFactory
