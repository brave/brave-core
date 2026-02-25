// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/page_action/brave_page_action_icon_type.h"

#define IsPageActionMigrated IsPageActionMigrated_Chromium

#include <chrome/browser/ui/page_action/page_action_icon_type.cc>

#undef IsPageActionMigrated

bool IsPageActionMigrated(PageActionIconType page_action) {
  if (page_action == brave::kPartitionedStorageActionIconType) {
    // Partitioned Storage (container) page action is based on the new framework
    // for page action.
    return true;
  }

  return IsPageActionMigrated_Chromium(page_action);
}
