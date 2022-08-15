/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/sync/chrome_sync_client.h"

#include "build/build_config.h"
#include "build/buildflag.h"
#include "extensions/buildflags/buildflags.h"

#if !BUILDFLAG(ENABLE_EXTENSIONS)

#define CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_PROVIDER_H_
#define CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_SYNC_BRIDGE_H_

#endif  // !BUILDFLAG(ENABLE_EXTENSIONS)

#define CreateDataTypeControllers CreateDataTypeControllers_ChromiumImpl

#include "src/chrome/browser/sync/chrome_sync_client.cc"

#undef CreateDataTypeControllers

namespace browser_sync {

syncer::DataTypeController::TypeVector
ChromeSyncClient::CreateDataTypeControllers(syncer::SyncService* sync_service) {
  syncer::DataTypeController::TypeVector controllers =
      CreateDataTypeControllers_ChromiumImpl(sync_service);

#if BUILDFLAG(IS_ANDROID)
  const base::RepeatingClosure dump_stack = GetDumpStackClosure();

  syncer::RepeatingModelTypeStoreFactory model_type_store_factory =
      GetModelTypeStoreService()->GetStoreFactory();

  controllers.push_back(
      std::make_unique<syncer::SyncableServiceBasedModelTypeController>(
          syncer::SEARCH_ENGINES, model_type_store_factory,
          GetSyncableServiceForType(syncer::SEARCH_ENGINES),
          std::move(dump_stack)));
#endif

  return controllers;
}

}  // namespace browser_sync
