/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/bookmarks/brave_bookmarks_side_panel_coordinator.h"

#include "base/functional/bind.h"
#include "brave/browser/ui/views/side_panel/brave_bookmarks_side_panel_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry.h"
#include "chrome/browser/ui/views/side_panel/side_panel_registry.h"

BraveBookmarksSidePanelCoordinator::~BraveBookmarksSidePanelCoordinator() =
    default;

void BraveBookmarksSidePanelCoordinator::CreateAndRegisterEntry(
    SidePanelRegistry* global_registry) {
  global_registry->Register(std::make_unique<SidePanelEntry>(
      SidePanelEntry::Key(SidePanelEntry::Id::kBookmarks),
      base::BindRepeating(
          &BraveBookmarksSidePanelCoordinator::CreateBookmarksPanelView,
          base::Unretained(this)),
      SidePanelEntry::kSidePanelDefaultContentWidth));
}

std::unique_ptr<views::View>
BraveBookmarksSidePanelCoordinator::CreateBookmarksPanelView(
    SidePanelEntryScope& scope) {
  return std::make_unique<BraveBookmarksSidePanelView>(scope);
}
