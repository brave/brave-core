/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 3.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/vendor/brave-ios/components/brave_sync/brave_sync_service.h"

#include "brave/vendor/brave-ios/components/bookmarks/bookmarks_api.h"
#include "ios/chrome/browser/bookmarks/bookmark_model_factory.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/undo/bookmark_undo_service_factory.h"

using ios::BookmarkModelFactory;
using ios::BookmarkUndoServiceFactory;

BraveSyncService::BraveSyncService(ChromeBrowserState* browser_state) {
  auto* model =
      BookmarkModelFactory::GetInstance()->GetForBrowserState(browser_state);

  auto* undo_service =
      BookmarkUndoServiceFactory::GetInstance()->GetForBrowserState(
          browser_state);

  bookmarks_api_ =
      std::make_unique<bookmarks::BookmarksAPI>(model, undo_service);
}

BraveSyncService::~BraveSyncService() {}

bookmarks::BookmarksAPI* BraveSyncService::bookmarks_api() {
    return bookmarks_api_.get();
}
