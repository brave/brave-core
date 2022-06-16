/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ios/chrome/browser/sync/ios_chrome_sync_client.h"

#include "components/sync/driver/sync_user_settings.h"

#define IOSChromeSyncClient IOSChromeSyncClient_ChromiumImpl
#include "src/ios/chrome/browser/sync/ios_chrome_sync_client.mm"
#undef IOSChromeSyncClient

IOSChromeSyncClient::~IOSChromeSyncClient() = default;

syncer::DataTypeController::TypeVector
IOSChromeSyncClient::CreateDataTypeControllers(
    syncer::SyncService* sync_service) {
  syncer::ModelTypeSet disabled_types = {
      syncer::PROXY_TABS, syncer::AUTOFILL,
      // syncer::PREFERENCES,
      syncer::READING_LIST, syncer::SEND_TAB_TO_SELF, syncer::USER_CONSENTS};
  return component_factory_->CreateCommonDataTypeControllers(disabled_types,
                                                             sync_service);
}

void IOSChromeSyncClient::SetDefaultEnabledTypes(
    syncer::SyncService* sync_service) {
  DCHECK(sync_service);

  syncer::UserSelectableTypeSet selected_types;
  selected_types.Put(syncer::UserSelectableType::kBookmarks);
  sync_service->GetUserSettings()->SetSelectedTypes(false, selected_types);
}
