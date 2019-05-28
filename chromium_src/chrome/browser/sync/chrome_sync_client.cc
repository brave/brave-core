/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/client/brave_sync_client_impl.h"
#include "../../../../../chrome/browser/sync/chrome_sync_client.cc"   // NOLINT

namespace browser_sync {

brave_sync::BraveSyncClient* ChromeSyncClient::GetBraveSyncClient() {
  return brave_sync_client_.get();
}

}   // namespace browser_sync
