/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BOOKMARKS_IMPORTER_IMPORTED_BOOKMARK_ENTRY_H_
#define BRAVE_IOS_BROWSER_API_BOOKMARKS_IMPORTER_IMPORTED_BOOKMARK_ENTRY_H_

#include <vector>

#include "base/time/time.h"
#include "url/gurl.h"

struct ImportedBookmarkEntry {
  ImportedBookmarkEntry();
  ImportedBookmarkEntry(const ImportedBookmarkEntry& other);
  ~ImportedBookmarkEntry();

  bool operator==(const ImportedBookmarkEntry& other) const;

  bool in_toolbar;
  bool is_folder;
  GURL url;
  std::vector<std::u16string> path;
  std::u16string title;
  base::Time creation_time;
};

#endif  // BRAVE_IOS_BROWSER_API_BOOKMARKS_IMPORTER_IMPORTED_BOOKMARK_ENTRY_H_
