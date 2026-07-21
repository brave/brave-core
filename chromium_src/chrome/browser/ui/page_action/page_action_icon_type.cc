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
    // button in the toolbar). Keeping it on the legacy path lets the
    // star_view.cc patch keep it hidden, matching the behavior from before
    // upstream enabled kPageActionsMigrationBookmarkStar by default in cr151.
    return false;
  }

  return IsPageActionMigrated_Chromium(page_action);
}
