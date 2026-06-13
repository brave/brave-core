/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/sync/brave_sync_service_impl_delegate.h"
#include "brave/components/brave_news/common/buildflags/buildflags.h"
#include "brave/components/sync/service/brave_sync_service_impl.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/sync/device_info_sync_service_factory.h"

#if BUILDFLAG(ENABLE_BRAVE_NEWS)
#include "brave/browser/brave_news/brave_news_controller_factory.h"
#include "brave/components/brave_news/browser/brave_news_controller.h"
#include "components/sync/base/data_type.h"
#include "components/sync/service/data_type_controller.h"
#endif

#define BRAVE_BUILD_SERVICE_INSTANCE_FOR                        \
  std::make_unique<syncer::BraveSyncServiceImpl>(               \
      std::move(init_params),                                   \
      std::make_unique<syncer::BraveSyncServiceImplDelegate>(   \
          DeviceInfoSyncServiceFactory::GetForProfile(profile), \
          HistoryServiceFactory::GetForProfile(                 \
              profile, ServiceAccessType::IMPLICIT_ACCESS)),    \
      g_browser_process->os_crypt_async());

#if BUILDFLAG(ENABLE_BRAVE_NEWS)
// Registers the BraveNewsControllerFactory dependency so the keyed service
// (and its sync bridge) outlives the SyncService.
#define BRAVE_SYNC_SERVICE_FACTORY_DEPENDS_ON \
  DependsOn(brave_news::BraveNewsControllerFactory::GetInstance());

// Appends the syncer::BRAVE_NEWS controller, wrapping the bridge delegate that
// BraveNewsController exposes. Null-checked for tests / non-regular profiles.
#define BRAVE_SYNC_SERVICE_FACTORY_APPEND_CONTROLLERS                        \
  if (auto* brave_news_controller =                                         \
          brave_news::BraveNewsControllerFactory::GetForBrowserContext(     \
              profile)) {                                                    \
    if (auto brave_news_delegate =                                          \
            brave_news_controller->GetSyncControllerDelegate()) {           \
      controllers.push_back(std::make_unique<syncer::DataTypeController>(    \
          syncer::BRAVE_NEWS, std::move(brave_news_delegate),               \
          /*delegate_for_transport_mode=*/nullptr));                        \
    }                                                                       \
  }
#else
#define BRAVE_SYNC_SERVICE_FACTORY_DEPENDS_ON
#define BRAVE_SYNC_SERVICE_FACTORY_APPEND_CONTROLLERS
#endif

#include <chrome/browser/sync/sync_service_factory.cc>

#undef BRAVE_SYNC_SERVICE_FACTORY_APPEND_CONTROLLERS
#undef BRAVE_SYNC_SERVICE_FACTORY_DEPENDS_ON
#undef BRAVE_BUILD_SERVICE_INSTANCE_FOR
