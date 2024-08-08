/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string_view>

#include "brave/components/sync/service/brave_sync_service_impl.h"
#include "chrome/browser/sync/sync_service_factory.h"
#include "components/sync/service/sync_user_settings.h"

// Include to prevent redefining HasPrimaryAccount method at the header
#include "components/signin/public/identity_manager/identity_manager.h"

// Include to prevent redefining IdentityManager method at the header
#include "chrome/browser/signin/identity_manager_factory.h"

class Profile;

namespace {

bool StopSyncIfActive(Profile* profile, const char* func_name) {
  // If HasPrimaryAccount is used somewhere out of DisableSyncForProfileDeletion
  // we don't want to stop Sync.
  if (std::string_view(func_name) != "DisableSyncForProfileDeletion") {
    VLOG(0) << "Unexpected call of StopSyncIfActive from " << func_name
            << " Sync is not stopped";
    return true;
  }

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

// Ensure HasPrimaryAccount override happens at DisableSyncForProfileDeletion
// function. We assume 'IdentityManager* identity_manager' is initialized from
// the factory followed by a nullptr check.
#define IdentityManager                                                        \
  IdentityManager* identity_manager_unused = nullptr;                          \
  (void)(identity_manager_unused);                                             \
  static_assert(std::string_view(__func__) == "DisableSyncForProfileDeletion", \
                "Override at a wrong function");                               \
  signin::IdentityManager

#define HasPrimaryAccount(LEVEL) \
  HasPrimaryAccount(LEVEL) && StopSyncIfActive(profile, __func__)

#include "src/chrome/browser/profiles/delete_profile_helper.cc"

#undef HasPrimaryAccount
#undef IdentityManager
#pragma clang diagnostic pop
