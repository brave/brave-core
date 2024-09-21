/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/sync/brave_sync_service_impl_delegate.h"
#include "brave/components/sync/service/brave_sync_service_impl.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/sync/device_info_sync_service_factory.h"

#define BRAVE_BUILD_SERVICE_INSTANCE_FOR                        \
  std::make_unique<syncer::BraveSyncServiceImpl>(               \
      std::move(init_params),                                   \
      std::make_unique<syncer::BraveSyncServiceImplDelegate>(   \
          DeviceInfoSyncServiceFactory::GetForProfile(profile), \
          HistoryServiceFactory::GetForProfile(                 \
              profile, ServiceAccessType::IMPLICIT_ACCESS)));

#include "src/chrome/browser/sync/sync_service_factory.cc"

#undef BRAVE_BUILD_SERVICE_INSTANCE_FOR
