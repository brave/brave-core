/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/bookmarks/bookmark_bar_controller.h"

#include "components/bookmarks/browser/bookmark_model.h"

// To prevent early return due to empty items in bookmark bar.
// We want to show bookmark bar with import instruction when it's empty.
#define HasBookmarks() HasBookmarks() || true

#include <chrome/browser/ui/bookmarks/bookmark_bar_controller.cc>

#undef HasBookmarks
