/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/tabs/saved_tab_groups/tab_group_sync_delegate_desktop.h"

// Pre-include headers that use ConnectLocalTabGroup so their include guards
// prevent the rename macro from affecting them.
#include "components/saved_tab_groups/internal/tab_group_sync_service_impl.h"

#define ConnectLocalTabGroup ConnectLocalTabGroup_ChromiumImpl

#include <chrome/browser/ui/tabs/saved_tab_groups/tab_group_sync_delegate_desktop.cc>

#undef ConnectLocalTabGroup

namespace tab_groups {

// The sync service may complete initialization during browser shutdown,
// triggering ConnectLocalTabGroup after the browser is destroyed. The
// upstream GetTabStripModelForLocalGroup CHECKs that the browser is
// non-null, which crashes. Guard against this by returning early when
// the browser for the group no longer exists.
void TabGroupSyncDelegateDesktop::ConnectLocalTabGroup(
    const SavedTabGroup& group) {
  if (group.local_group_id().has_value()) {
    if (!SavedTabGroupUtils::GetBrowserWithTabGroupId(
            group.local_group_id().value())) {
      return;
    }
  }
  ConnectLocalTabGroup_ChromiumImpl(group);
}

}  // namespace tab_groups
