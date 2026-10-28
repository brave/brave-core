/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/workspaces/workspace_session_utils.h"

#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "brave/browser/workspaces/workspace_metadata.h"
#include "chrome/browser/ui/browser_tabrestore.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
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

std::vector<std::unique_ptr<sessions::SessionCommand>>
SerializeWindowForWorkspace(BrowserWindowInterface* window,
                            WorkspaceMetadata& workspace) {
  TabStripModel* tsm = window->GetTabStripModel();
  workspace.number_of_tabs = tsm->count();
  if (tsm->count() == 0) {
    return {};
  }

  // RestoreSessionFromCommands drops windows whose type it never saw, so the
  // commands are still scoped to a window ID even though a workspace only ever
  // describes one.  The ID is local to this serialization.
  const SessionID window_id = SessionID::NewUnique();

  std::vector<std::unique_ptr<sessions::SessionCommand>> commands;
  commands.push_back(sessions::CreateSetWindowTypeCommand(
      window_id, sessions::SessionWindow::TYPE_NORMAL));

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

  return commands;
}

void RestoreWorkspaceIntoWindow(
    BrowserWindowInterface* window,
    std::vector<std::unique_ptr<sessions::SessionCommand>> commands) {
  // RestoreSessionFromCommands constructs SessionTab/SessionWindow objects
  // whose constructors call SessionID::NewUnique(), which is sequence-checked
  // to the UI thread.  It must therefore be called here (UI thread), not in
  // the background I/O task.
  std::vector<std::unique_ptr<sessions::SessionWindow>> windows;
  SessionID active_window_id = SessionID::InvalidValue();
  std::string platform_session_id;
  std::set<SessionID> discarded_window_ids;
  sessions::RestoreSessionFromCommands(commands, &windows, &active_window_id,
                                       &platform_session_id,
                                       &discarded_window_ids);
  if (windows.empty() || windows.front()->tabs.empty()) {
    return;
  }

  // A workspace describes exactly one window's worth of tabs.
  const sessions::SessionWindow& session_window = *windows.front();
  TabStripModel* tsm = window->GetTabStripModel();

  // Append after whatever is already open so this function never destroys
  // tabs; emptying the window first is the caller's decision.
  const int base_index = tsm->count();

  // We restore tabs ourselves rather than using RestoreForeignSessionWindows
  // because that API creates an empty new_group_ids map and never calls
  // RestoreTabGroupMetadata, so tab group names/colors are silently dropped.
  // By passing the original TabGroupId directly to AddRestoredTab, we avoid
  // any ID remapping and can apply visual data with the same IDs afterwards.
  for (size_t i = 0; i < session_window.tabs.size(); ++i) {
    const auto& tab = session_window.tabs[i];
    if (tab->navigations.empty()) {
      continue;
    }
    chrome::AddRestoredTab(
        window, tab->navigations,
        /*tab_index=*/base_index + static_cast<int>(i),
        tab->normalized_navigation_index(), tab->extension_app_id,
        /*group=*/tab->group,
        /*select=*/false, tab->pinned,
        /*last_active_time_ticks=*/base::TimeTicks(), tab->last_active_time,
        /*storage_namespace=*/nullptr, tab->user_agent_override,
        tab->extra_data,
        /*from_session_restore=*/true,
        /*is_active_browser=*/std::nullopt);
  }

  // Apply group names, colors, and collapsed state.  Since we passed the
  // original TabGroupIds to AddRestoredTab, no ID remapping is needed.
  for (const auto& session_group : session_window.tab_groups) {
    if (!tsm->group_model() ||
        !tsm->group_model()->ContainsTabGroup(session_group->id)) {
      continue;
    }
    tsm->ChangeTabGroupVisuals(session_group->id, session_group->visual_data);
  }

  const int active = base_index + session_window.selected_tab_index;
  if (active >= base_index && active < tsm->count()) {
    tsm->ActivateTabAt(active);
  }
}
