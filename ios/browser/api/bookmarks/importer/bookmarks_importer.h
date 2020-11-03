/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BOOKMARKS_IMPORTER_BOOKMARKS_IMPORTER_H_
#define BRAVE_IOS_BROWSER_API_BOOKMARKS_IMPORTER_BOOKMARKS_IMPORTER_H_

#include "brave/ios/browser/api/bookmarks/importer/imported_bookmark_entry.h"

#include <vector>
#include "base/strings/string16.h"

class BookmarksImporter {
public:
  // top_level_folder_name is usually set to IDS_BOOKMARK_GROUP
  // Which is the name of the folder bookmarks will be imported into,
  // if the root folder is not empty.
  static void AddBookmarks(const base::string16& top_level_folder_name,
                           const std::vector<ImportedBookmarkEntry>& bookmarks);
};

#endif  // BRAVE_IOS_BROWSER_API_BOOKMARKS_IMPORTER_BOOKMARKS_IMPORTER_H_
