/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/workspace/workspace_session_utils.h"

#include <memory>
#include <vector>

#include "chrome/browser/ui/tabs/tab_group_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/sessions/content/content_serialized_navigation_builder.h"
#include "components/sessions/core/session_service_commands.h"
#include "components/sessions/core/session_types.h"
#include "components/tab_groups/tab_group_visual_data.h"
#include "components/tabs/public/tab_group.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"

void AppendBrowserSessionCommands(
    const SessionID& window_id,
    TabStripModel* tsm,
    gfx::Rect restored_bounds,
    ui::mojom::WindowShowState restored_state,
    std::vector<std::unique_ptr<sessions::SessionCommand>>& commands) {
  commands.push_back(sessions::CreateSetWindowTypeCommand(
      window_id, sessions::SessionWindow::TYPE_NORMAL));
  commands.push_back(sessions::CreateSetWindowBoundsCommand(
      window_id, restored_bounds, restored_state));

  // Emit group metadata for every tab group in this window.
  if (tsm->group_model()) {
    for (const tab_groups::TabGroupId& group_id :
         tsm->group_model()->ListTabGroups()) {
      auto* group = tsm->group_model()->GetTabGroup(group_id);
      if (!group || !group->visual_data()) {
        continue;
      }
      commands.push_back(sessions::CreateTabGroupMetadataUpdateCommand(
          group_id, group->visual_data()));
    }
  }

  for (int i = 0; i < tsm->count(); ++i) {
    content::WebContents* contents = tsm->GetWebContentsAt(i);
    CHECK(contents);

    SessionID tab_id = SessionID::NewUnique();

    commands.push_back(sessions::CreateSetTabWindowCommand(window_id, tab_id));
    commands.push_back(sessions::CreateSetTabIndexInWindowCommand(tab_id, i));

    if (tsm->IsTabPinned(i)) {
      commands.push_back(sessions::CreatePinnedStateCommand(tab_id, true));
    }

    std::optional<tab_groups::TabGroupId> group_id = tsm->GetTabGroupForTab(i);
    if (group_id.has_value()) {
      commands.push_back(sessions::CreateTabGroupCommand(tab_id, group_id));
    }

    // Serialize the full navigation history for this tab.
    auto& controller = contents->GetController();
    int nav_count = controller.GetEntryCount();
    int current_entry = controller.GetCurrentEntryIndex();
    for (int j = 0; j < nav_count; ++j) {
      content::NavigationEntry* entry = controller.GetEntryAtIndex(j);
      if (!entry) {
        continue;
      }
      auto serialized =
          sessions::ContentSerializedNavigationBuilder::FromNavigationEntry(
              j, entry);
      commands.push_back(
          sessions::CreateUpdateTabNavigationCommand(tab_id, serialized));
    }

    commands.push_back(sessions::CreateSetSelectedNavigationIndexCommand(
        tab_id, current_entry));
  }

  commands.push_back(sessions::CreateSetSelectedTabInWindowCommand(
      window_id, tsm->active_index()));
}
