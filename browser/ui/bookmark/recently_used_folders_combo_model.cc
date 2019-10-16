/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/bookmark/recently_used_folders_combo_model.h"

#include "components/bookmarks/browser/bookmark_model.h"

BraveRecentlyUsedFoldersComboModel::BraveRecentlyUsedFoldersComboModel(
    bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* node)
    : RecentlyUsedFoldersComboModel(model, node) {
  RecentlyUsedFoldersComboModel::RemoveNode(model->other_node());
}

BraveRecentlyUsedFoldersComboModel::~BraveRecentlyUsedFoldersComboModel() {}
