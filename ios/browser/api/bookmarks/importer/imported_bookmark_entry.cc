/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/bookmarks/importer/imported_bookmark_entry.h"

ImportedBookmarkEntry::ImportedBookmarkEntry()
    : in_toolbar(false),
      is_folder(false) {}

ImportedBookmarkEntry::ImportedBookmarkEntry(
    const ImportedBookmarkEntry& other) = default;

ImportedBookmarkEntry::~ImportedBookmarkEntry() {}

bool ImportedBookmarkEntry::operator==(
    const ImportedBookmarkEntry& other) const {
  return (in_toolbar == other.in_toolbar &&
          is_folder == other.is_folder &&
          url == other.url &&
          path == other.path &&
          title == other.title &&
          creation_time == other.creation_time);
}
