/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/chromium_src/chrome/browser/sync/profile_sync_service_factory.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "brave/components/brave_sync/brave_profile_sync_service.h"
using brave_sync::BraveProfileSyncService;
#endif
#include "../../../../../chrome/browser/sync/profile_sync_service_factory.cc"   // NOLINT

#if BUILDFLAG(ENABLE_EXTENSIONS)
// static
BraveProfileSyncService*
ProfileSyncServiceFactory::GetAsBraveProfileSyncServiceForProfile(
    Profile* profile) {
  return static_cast<BraveProfileSyncService*>(GetForProfile(profile));
}
#endif
