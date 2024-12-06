/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_BOOKMARKS_SIDE_PANEL_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_BOOKMARKS_SIDE_PANEL_VIEW_H_

#include "brave/browser/ui/views/side_panel/brave_side_panel_view_base.h"

class Browser;
class SidePanelEntryScope;

// Gives bookmarks panel specific header view with web view.
class BraveBookmarksSidePanelView : public BraveSidePanelViewBase {
 public:
  explicit BraveBookmarksSidePanelView(Browser* browser,
                                       SidePanelEntryScope& scope);
  ~BraveBookmarksSidePanelView() override;
  BraveBookmarksSidePanelView(const BraveBookmarksSidePanelView&) = delete;
  BraveBookmarksSidePanelView& operator=(const BraveBookmarksSidePanelView&) =
      delete;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_BOOKMARKS_SIDE_PANEL_VIEW_H_
