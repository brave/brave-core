/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/sync/service/brave_sync_service_impl.h"
#include "chrome/browser/sync/sync_service_factory.h"
#include "components/sync/service/sync_user_settings.h"

// Include to prevent redefining HasPrimaryAccount method at the header
#include "components/signin/public/identity_manager/identity_manager.h"

class Profile;

namespace {

bool StopSyncIfActive(Profile* profile) {
  if (SyncServiceFactory::HasSyncService(profile)) {
    syncer::BraveSyncServiceImpl* sync_service =
        static_cast<syncer::BraveSyncServiceImpl*>(
            SyncServiceFactory::GetForProfile(profile));
    sync_service->StopAndClear();
  }

  return true;
}

}  // namespace

// Suppress warning after macro expanding
// error: '&&' within '||' [-Werror,-Wlogical-op-parentheses]
// if (!identity_manager || !identity_manager->HasPrimaryAccount(..) &&
//     !StopSyncIfActive(profile)) {
// Compiler want us to make
// if (!A || (!B && C)), but with this type of override we can't.

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlogical-op-parentheses"

#define HasPrimaryAccount(LEVEL) \
  HasPrimaryAccount(LEVEL) && StopSyncIfActive(profile)

#include "src/chrome/browser/profiles/delete_profile_helper.cc"

#undef HasPrimaryAccount
#pragma clang diagnostic pop
