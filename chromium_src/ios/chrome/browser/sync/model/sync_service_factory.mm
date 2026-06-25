/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/containers/extend.h"
#include "brave/browser/sync/brave_sync_service_impl_delegate.h"
#include "brave/components/sync/service/brave_sync_service_impl.h"
#include "components/sync/service/data_type_controller.h"
#include "ios/chrome/browser/history/model/history_service_factory.h"
#include "ios/chrome/browser/sync/model/device_info_sync_service_factory.h"

class ProfileIOS;

namespace syncer {
class SyncService;
}  // namespace syncer

// Forward declared to avoid adding a compile-time dependency.
// Implementation is provided by //brave/ios/browser/sync:chromium_impl.
syncer::DataTypeController::TypeVector CreateBraveControllers(
    ProfileIOS* profile,
    syncer::SyncService* sync_service);

#define BRAVE_BUILD_SERVICE_INSTANCE_FOR                        \
  std::make_unique<syncer::BraveSyncServiceImpl>(               \
      std::move(init_params),                                   \
      std::make_unique<syncer::BraveSyncServiceImplDelegate>(   \
          DeviceInfoSyncServiceFactory::GetForProfile(profile), \
          ios::HistoryServiceFactory::GetForProfile(            \
              profile, ServiceAccessType::IMPLICIT_ACCESS)),    \
      GetApplicationContext()->GetOSCryptAsync());

#define BRAVE_CREATE_CONTROLLERS \
  base::Extend(controllers, CreateBraveControllers(profile, sync_service));

#include <ios/chrome/browser/sync/model/sync_service_factory.mm>

#undef BRAVE_CREATE_CONTROLLERS
#undef BRAVE_BUILD_SERVICE_INSTANCE_FOR
