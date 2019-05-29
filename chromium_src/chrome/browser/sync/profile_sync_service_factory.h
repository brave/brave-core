/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SYNC_PROFILE_SYNC_SERVICE_FACTORY_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SYNC_PROFILE_SYNC_SERVICE_FACTORY_H_

#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
namespace brave_sync {
class BraveProfileSyncService;
}   // namespace brave_sync
#endif
#include "../../../../../chrome/browser/sync/profile_sync_service_factory.h"

#endif    // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SYNC_PROFILE_SYNC_SERVICE_FACTORY_H_   // NOLINT
