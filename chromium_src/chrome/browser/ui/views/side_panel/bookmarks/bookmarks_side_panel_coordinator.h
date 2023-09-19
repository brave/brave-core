// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_BOOKMARKS_BOOKMARKS_SIDE_PANEL_COORDINATOR_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_BOOKMARKS_BOOKMARKS_SIDE_PANEL_COORDINATOR_H_

#define CreateAndRegisterEntry              \
  CreateAndRegisterEntry_UnUsed() {}        \
  friend class BraveBookmarksSidePanelView; \
  void CreateAndRegisterEntry

#include "src/chrome/browser/ui/views/side_panel/bookmarks/bookmarks_side_panel_coordinator.h"  // IWYU pragma: export

#undef CreateAndRegisterEntry

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_BOOKMARKS_BOOKMARKS_SIDE_PANEL_COORDINATOR_H_
