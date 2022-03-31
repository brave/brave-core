/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_SYNC_SERVICE_IMPL_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_SYNC_SERVICE_IMPL_HELPER_H_

#include <string>

#include "base/callback.h"

namespace syncer {
class BraveSyncServiceImpl;
class DeviceInfoSyncService;
}  // namespace syncer

namespace brave_sync {

// Helper function to break circular dependency between //components/sync/driver
// and //component/sync_device_info
void ResetSync(syncer::BraveSyncServiceImpl* sync_service_impl,
               syncer::DeviceInfoSyncService* device_info_service,
               base::OnceClosure on_reset_done);

void DeleteDevice(syncer::BraveSyncServiceImpl* sync_service_impl,
                  syncer::DeviceInfoSyncService* device_info_service,
                  const std::string& device_guid);

}  // namespace brave_sync

#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_SYNC_SERVICE_IMPL_HELPER_H_
