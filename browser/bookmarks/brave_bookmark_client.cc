/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/bookmarks/brave_bookmark_client.h"

BraveBookmarkClient::BraveBookmarkClient(
    Profile* profile,
    bookmarks::ManagedBookmarkService* managed_bookmark_service,
    sync_bookmarks::BookmarkSyncService* bookmark_sync_service) :
  ChromeBookmarkClient(profile, managed_bookmark_service,
                       bookmark_sync_service) {
}

BraveBookmarkClient::~BraveBookmarkClient() {}

bool BraveBookmarkClient::IsPermanentNodeVisible(
      const bookmarks::BookmarkPermanentNode* node) {
  if (node->type() == bookmarks::BookmarkNode::OTHER_NODE)
    return false;
  return ChromeBookmarkClient::IsPermanentNodeVisible(node);
}
