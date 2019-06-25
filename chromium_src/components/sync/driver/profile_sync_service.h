/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DRIVER_PROFILE_SYNC_SERVICE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DRIVER_PROFILE_SYNC_SERVICE_H_

namespace brave_sync {
class BraveProfileSyncServiceImpl;
}   // namesapce brave_sync

#define BRAVE_PROFILE_SYNC_SERVICE_H \
friend class brave_sync::BraveProfileSyncServiceImpl;

#include "../../../../../components/sync/driver/profile_sync_service.h"
#undef BRAVE_PROFILE_SYNC_SERVICE_H

#endif    // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DRIVER_PROFILE_SYNC_SERVICE_H_
