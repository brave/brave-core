/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BOOKMARK_RECENTLY_USED_FOLDERS_COMBO_MODEL_H_
#define BRAVE_BROWSER_UI_BOOKMARK_RECENTLY_USED_FOLDERS_COMBO_MODEL_H_

#include "chrome/browser/ui/bookmarks/recently_used_folders_combo_model.h"

class BraveRecentlyUsedFoldersComboModel
    : public RecentlyUsedFoldersComboModel {
 public:
  BraveRecentlyUsedFoldersComboModel(bookmarks::BookmarkModel* model,
                                     const bookmarks::BookmarkNode* node);
  ~BraveRecentlyUsedFoldersComboModel() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveRecentlyUsedFoldersComboModel);
};

#endif  // BRAVE_BROWSER_UI_BOOKMARK_RECENTLY_USED_FOLDERS_COMBO_MODEL_H_
