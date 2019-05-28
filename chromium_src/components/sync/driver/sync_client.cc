/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../../components/sync/driver/sync_client.cc"   // NOLINT

#include "brave/chromium_src/components/sync/driver/sync_client.h"

namespace syncer {

brave_sync::BraveSyncClient* SyncClient::GetBraveSyncClient() {
  return nullptr;
}

}
