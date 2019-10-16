/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BOOKMARKS_BRAVE_BOOKMARK_CLIENT_H_
#define BRAVE_BROWSER_BOOKMARKS_BRAVE_BOOKMARK_CLIENT_H_

#include "chrome/browser/bookmarks/chrome_bookmark_client.h"

class BraveBookmarkClient : public ChromeBookmarkClient {
 public:
  BraveBookmarkClient(
      Profile* profile,
      bookmarks::ManagedBookmarkService* managed_bookmark_service,
      sync_bookmarks::BookmarkSyncService* bookmark_sync_service);
  ~BraveBookmarkClient() override;

  // bookmarks::BookmarkClient:
  bool IsPermanentNodeVisible(
      const bookmarks::BookmarkPermanentNode* node) override;
 private:
  DISALLOW_COPY_AND_ASSIGN(BraveBookmarkClient);
};

#endif  // BRAVE_BROWSER_BOOKMARKS_BRAVE_BOOKMARK_CLIENT_H_
