/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BOOKMARKS_BRAVE_BOOKMARKS_SIDE_PANEL_COORDINATOR_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BOOKMARKS_BRAVE_BOOKMARKS_SIDE_PANEL_COORDINATOR_H_

#include <memory>

#include "chrome/browser/ui/views/side_panel/bookmarks/bookmarks_side_panel_coordinator.h"

class SidePanelEntryScope;
class SidePanelRegistry;

namespace views {
class View;
}  // namespace views

// Introduced to give custom contents view(BraveBookmarksSidePanelView) for
// bookmarks panel entry. That contents view includes bookmarks panel specific
// header view and web view.
// BookmarksSidePanelCoordinator::CreateBookmarksWebView() is reused from
// BraveBookmarksSidePanelView. That's why BraveBookmarksSidePanelView is set
// as BookmarksSidePanelCoordinator's friend.
class BraveBookmarksSidePanelCoordinator
    : public BookmarksSidePanelCoordinator {
 public:
  using BookmarksSidePanelCoordinator::BookmarksSidePanelCoordinator;
  ~BraveBookmarksSidePanelCoordinator() override;

  void CreateAndRegisterEntry(SidePanelRegistry* global_registry) override;

 private:
  std::unique_ptr<views::View> CreateBookmarksPanelView(
      SidePanelEntryScope& scope);
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BOOKMARKS_BRAVE_BOOKMARKS_SIDE_PANEL_COORDINATOR_H_
