/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/bookmarks/bookmark_editor.h"

// Prefer an explicit window_title_id when set (used for "Bookmark selected
// tabs") over Chromium's NEW_FOLDER-with-children title ("Bookmark all tabs").
#define GetWindowTitleId GetWindowTitleId_ChromiumImpl
#include <chrome/browser/ui/bookmarks/bookmark_editor.cc>
#undef GetWindowTitleId

int BookmarkEditor::EditDetails::GetWindowTitleId() const {
  if (window_title_id.has_value()) {
    return *window_title_id;
  }
  return GetWindowTitleId_ChromiumImpl();
}
