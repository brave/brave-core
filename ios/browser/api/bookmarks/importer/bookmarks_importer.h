/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BOOKMARKS_IMPORTER_BOOKMARKS_IMPORTER_H_
#define BRAVE_IOS_BROWSER_API_BOOKMARKS_IMPORTER_BOOKMARKS_IMPORTER_H_

#include "brave/ios/browser/api/bookmarks/importer/imported_bookmark_entry.h"

#include <vector>

class BookmarksImporter {
public:
  static void AddBookmarks(const std::vector<ImportedBookmarkEntry>& bookmarks);
};

#endif  // BRAVE_IOS_BROWSER_API_BOOKMARKS_IMPORTER_BOOKMARKS_IMPORTER_H_
