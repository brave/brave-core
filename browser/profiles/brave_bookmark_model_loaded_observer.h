/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PROFILES_BRAVE_BOOKMARK_MODEL_LOADED_OBSERVER_H_
#define BRAVE_BROWSER_PROFILES_BRAVE_BOOKMARK_MODEL_LOADED_OBSERVER_H_

#include "chrome/browser/profiles/bookmark_model_loaded_observer.h"

class BraveBookmarkModelLoadedObserver
    : public BookmarkModelLoadedObserver {
 public:
  explicit BraveBookmarkModelLoadedObserver(Profile* profile);
  BraveBookmarkModelLoadedObserver(const BraveBookmarkModelLoadedObserver&) =
      delete;
  BraveBookmarkModelLoadedObserver& operator=(
      const BraveBookmarkModelLoadedObserver&) = delete;

 private:
  void BookmarkModelLoaded(bookmarks::BookmarkModel* model,
                           bool ids_reassigned) override;
};

#endif  // BRAVE_BROWSER_PROFILES_BRAVE_BOOKMARK_MODEL_LOADED_OBSERVER_H_
