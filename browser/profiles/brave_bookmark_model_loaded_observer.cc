/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/profiles/brave_bookmark_model_loaded_observer.h"

#include "brave/common/pref_names.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/prefs/pref_service.h"

using bookmarks::BookmarkModel;

BraveBookmarkModelLoadedObserver::BraveBookmarkModelLoadedObserver(
    Profile* profile)
    : BookmarkModelLoadedObserver(profile) {}

void BraveBookmarkModelLoadedObserver::BookmarkModelLoaded(
    BookmarkModel* model,
    bool ids_reassigned) {
  if (!profile_->GetPrefs()->GetBoolean(kOtherBookmarksMigrated)) {
    BraveMigrateOtherNodeFolder(model);
    profile_->GetPrefs()->SetBoolean(kOtherBookmarksMigrated, true);
  }

  brave_sync::Prefs brave_sync_prefs(profile_->GetPrefs());
  if (!brave_sync_prefs.IsSyncV1MetaInfoCleared()) {
    BraveClearSyncV1MetaInfo(model);
    brave_sync_prefs.SetSyncV1MetaInfoCleared(true);
  }

  BookmarkModelLoadedObserver::BookmarkModelLoaded(model, ids_reassigned);
}
