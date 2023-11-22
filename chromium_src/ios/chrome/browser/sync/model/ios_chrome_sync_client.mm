/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ios/chrome/browser/sync/model/ios_chrome_sync_client.h"

#define IOSChromeSyncClient IOSChromeSyncClient_ChromiumImpl
#include "src/ios/chrome/browser/sync/model/ios_chrome_sync_client.mm"
#undef IOSChromeSyncClient

IOSChromeSyncClient::~IOSChromeSyncClient() = default;

syncer::DataTypeController::TypeVector
IOSChromeSyncClient::CreateDataTypeControllers(
    syncer::SyncService* sync_service) {
  syncer::ModelTypeSet disabled_types = {syncer::AUTOFILL, syncer::READING_LIST,
                                         syncer::USER_CONSENTS};
  return component_factory_->CreateCommonDataTypeControllers(disabled_types,
                                                             sync_service);
}
