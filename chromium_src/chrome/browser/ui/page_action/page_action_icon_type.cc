// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/page_action/page_action_icon_type.h"

#define IsPageActionMigrated IsPageActionMigrated_Chromium

#include <chrome/browser/ui/page_action/page_action_icon_type.cc>

#undef IsPageActionMigrated

bool IsPageActionMigrated(PageActionIconType page_action) {
  if (page_action == brave::kPartitionedStorageActionIconType) {
    // Partitioned Storage (container) page action is based on the new framework
    // for page action.
    return true;
  }

  if (page_action == brave::kPsstIconActionIconType) {
    // PSST page action is based on the new framework
    // for page action.
    return true;
  }

  if (page_action == PageActionIconType::kBookmarkStar) {
    // Brave hides the location bar bookmark star (we have our own bookmark
    // button in the toolbar). `kBookmarkStar` is now unconditionally on the new
    // framework upstream (`StarView` was deleted in cr152), so this no longer
    // selects a legacy code path. It still needs to return false, though:
    // `TabFeatures` only constructs a `BookmarkPageActionController`
    // (chrome/browser/ui/tabs/tab_features.cc) when this returns true, and that
    // controller is the only thing that ever calls
    // `PageActionController::Show()` for `kActionBookmarkThisTab`. Without it,
    // the action's visibility stays at its default (hidden).
    return false;
  }

  return IsPageActionMigrated_Chromium(page_action);
}
