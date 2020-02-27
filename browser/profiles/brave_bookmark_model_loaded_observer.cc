/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/profiles/brave_bookmark_model_loaded_observer.h"

#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/prefs/pref_service.h"

#include "brave/components/brave_sync/buildflags/buildflags.h"
#if BUILDFLAG(ENABLE_BRAVE_SYNC)
#include "brave/components/brave_sync/brave_profile_sync_service_impl.h"
using brave_sync::BraveProfileSyncServiceImpl;
#endif

using bookmarks::BookmarkModel;

BraveBookmarkModelLoadedObserver::BraveBookmarkModelLoadedObserver(
    Profile* profile)
    : BookmarkModelLoadedObserver(profile) {}

void BraveBookmarkModelLoadedObserver::BookmarkModelLoaded(
    BookmarkModel* model,
    bool ids_reassigned) {
  if (!profile_->GetPrefs()->GetBoolean(kOtherBookmarksMigrated)) {
#if BUILDFLAG(ENABLE_BRAVE_SYNC)
    BraveProfileSyncServiceImpl* brave_profile_service =
      static_cast<BraveProfileSyncServiceImpl*>(
          ProfileSyncServiceFactory::GetForProfile(profile_));
    // When sync is enabled, we need to send migration records to other devices
    // so it is handled in BraveProfileSyncServiceImpl::OnSyncReady
    if (!brave_profile_service ||
        (brave_profile_service && !brave_profile_service->IsBraveSyncEnabled()))
      BraveMigrateOtherNodeFolder(model);
#else
    BraveMigrateOtherNodeFolder(model);
#endif
    profile_->GetPrefs()->SetBoolean(kOtherBookmarksMigrated, true);
  }

#if BUILDFLAG(ENABLE_BRAVE_SYNC)
  BraveProfileSyncServiceImpl::AddNonClonedBookmarkKeys(model);
#endif

  BookmarkModelLoadedObserver::BookmarkModelLoaded(model, ids_reassigned);
}
