/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 3.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BRAVE_IOS_COMPONENTS_BOOKMARKS_BOOKMARKS_API_H_
#define BRAVE_VENDOR_BRAVE_IOS_COMPONENTS_BOOKMARKS_BOOKMARKS_API_H_

#include <set>
#include <vector>

#include "base/macros.h"
#include "base/sequenced_task_runner.h"
#include "url/gurl.h"
#include "components/bookmarks/browser/bookmark_client.h"

class GURL;
class ChromeBrowserState;

namespace sync_bookmarks {
class BookmarkSyncService;
}

namespace bookmarks {
class BookmarkModel;
class BookmarkPermanentNode;
}

namespace brave {
class BraveBookmarkClient: public bookmarks::BookmarkClient {
 public:
  BraveBookmarkClient(ChromeBrowserState* browser_state,
                      sync_bookmarks::BookmarkSyncService* bookmark_sync_service);
  ~BraveBookmarkClient() override;
    
  // bookmarks::BookmarkClient:
  void Init(bookmarks::BookmarkModel* model) override;
  bool PreferTouchIcon() override;
  base::CancelableTaskTracker::TaskId GetFaviconImageForPageURL(
      const GURL& page_url,
      favicon_base::IconType type,
      favicon_base::FaviconImageCallback callback,
      base::CancelableTaskTracker* tracker) override;
  bool SupportsTypedCountForUrls() override;
  void GetTypedCountForUrls(UrlTypedCountMap* url_typed_count_map) override;
  bool IsPermanentNodeVisible(
      const bookmarks::BookmarkPermanentNode* node) override;
  void RecordAction(const base::UserMetricsAction& action) override;
  bookmarks::LoadManagedNodeCallback GetLoadManagedNodeCallback() override;
  bool CanSetPermanentNodeTitle(
      const bookmarks::BookmarkNode* permanent_node) override;
  bool CanSyncNode(const bookmarks::BookmarkNode* node) override;
  bool CanBeEditedByUser(const bookmarks::BookmarkNode* node) override;
  std::string EncodeBookmarkSyncMetadata() override;
  void DecodeBookmarkSyncMetadata(
      const std::string& metadata_str,
      const base::RepeatingClosure& schedule_save_closure) override;

 private:
  // Pointer to the associated ios::ChromeBrowserState. Must outlive
  // BookmarkClientImpl.
  ChromeBrowserState* browser_state_;

  bookmarks::BookmarkModel* model_;

  // Pointer to the BookmarkSyncService responsible for encoding and decoding
  // sync metadata persisted together with the bookmarks model.
  sync_bookmarks::BookmarkSyncService* bookmark_sync_service_;

  DISALLOW_COPY_AND_ASSIGN(BraveBookmarkClient);
};
}

#endif  // BRAVE_VENDOR_BRAVE_IOS_COMPONENTS_BOOKMARKS_BOOKMARKS_API_H_
