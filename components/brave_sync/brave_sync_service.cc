/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/brave_sync_service.h"

#include "brave/components/brave_sync/brave_sync_service_observer.h"

namespace brave_sync {

BraveSyncService::BraveSyncService() {}
BraveSyncService::~BraveSyncService() {}

void BraveSyncService::AddObserver(BraveSyncServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void BraveSyncService::RemoveObserver(BraveSyncServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace brave_sync
