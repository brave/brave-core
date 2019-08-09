/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/bookmarks/brave_bookmark_client.h"

#include "brave/components/brave_sync/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/managed/managed_bookmark_service.h"
#include "components/sync_bookmarks/bookmark_sync_service.h"

#if BUILDFLAG(ENABLE_BRAVE_SYNC)
#include "brave/components/brave_sync/brave_sync_service.h"
#endif

BraveBookmarkClient::BraveBookmarkClient(
    Profile* profile,
    bookmarks::ManagedBookmarkService* managed_bookmark_service,
    sync_bookmarks::BookmarkSyncService* bookmark_sync_service)
    : ChromeBookmarkClient(profile,
                           managed_bookmark_service,
                           bookmark_sync_service) {}

bookmarks::LoadManagedNodeCallback
BraveBookmarkClient::GetLoadManagedNodeCallback() {
#if BUILDFLAG(ENABLE_BRAVE_SYNC)
  return base::BindOnce(&brave_sync::LoadManagedNode,
      ChromeBookmarkClient::GetLoadManagedNodeCallback());
#endif
  return ChromeBookmarkClient::GetLoadManagedNodeCallback();
}

bool BraveBookmarkClient::IsPermanentNodeVisible(
    const bookmarks::BookmarkPermanentNode* node) {
#if BUILDFLAG(ENABLE_BRAVE_SYNC)
  if (brave_sync::IsSyncManagedNode(node))
    return false;  // don't display sync managed nodes
#endif
  return ChromeBookmarkClient::IsPermanentNodeVisible(node);
}
