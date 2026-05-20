/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/workspaces/workspace_session_utils.h"

#include <algorithm>
#include <memory>
#include <set>
#include <vector>

#include "brave/browser/workspaces/workspace_metadata.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabrestore.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/global_browser_collection.h"
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

namespace {

// Appends session commands for a single browser window to |commands|.
// Serializes window type, bounds, tab groups, tabs (with full navigation
// history), pinned state, and the active tab index.
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

}  // namespace

std::vector<std::unique_ptr<sessions::SessionCommand>>
GenerateBrowserSessionCommandsForWorkspace(Profile* profile,
                                           WorkspaceMetadata& workspace) {
  std::vector<std::unique_ptr<sessions::SessionCommand>> commands;
  SessionID active_window_id = SessionID::InvalidValue();

  GlobalBrowserCollection::GetInstance()->ForEach(
      [&](BrowserWindowInterface* bwi) {
        if (bwi->GetProfile() != profile ||
            bwi->GetType() != BrowserWindowInterface::Type::TYPE_NORMAL) {
          return true;
        }

        int tabs = bwi->GetTabStripModel()->count();
        if (tabs == 0) {
          return true;
        }

        SessionID window_id = SessionID::NewUnique();
        auto* window = bwi->GetWindow();
        AppendBrowserSessionCommands(window_id, bwi->GetTabStripModel(),
                                     window->GetRestoredBounds(),
                                     window->GetRestoredState(), commands);

        // Prefer the focused window; fall back to the first non-empty window.
        if (window->IsActive() ||
            active_window_id == SessionID::InvalidValue()) {
          active_window_id = window_id;
        }
        workspace.number_of_windows++;
        workspace.number_of_tabs += tabs;
        return true;
      });

  if (workspace.number_of_tabs == 0) {
    return {};
  }

  if (active_window_id != SessionID::InvalidValue()) {
    commands.push_back(
        sessions::CreateSetActiveWindowCommand(active_window_id));
  }

  return commands;
}

void RestoreBrowserSessionCommandsForWorkspace(
    Profile* profile,
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
  if (windows.empty()) {
    return;
  }

  // We restore tabs ourselves rather than using RestoreForeignSessionWindows
  // because that API creates an empty new_group_ids map and never calls
  // RestoreTabGroupMetadata, so tab group names/colors are silently dropped.
  // By passing the original TabGroupId directly to AddRestoredTab, we avoid
  // any ID remapping and can apply visual data with the same IDs afterwards.
  for (const auto& window : windows) {
    if (window->tabs.empty()) {
      continue;
    }

    if (Browser::GetCreationStatusForProfile(profile) !=
        Browser::CreationStatus::kOk) {
      continue;
    }

    Browser::CreateParams params(Browser::TYPE_NORMAL, profile, false);
    params.initial_bounds = window->bounds;
    params.initial_show_state = window->show_state;
    params.initial_workspace = window->workspace;
    params.initial_visible_on_all_workspaces_state =
        window->visible_on_all_workspaces;
    params.should_trigger_session_restore = false;
    Browser* browser = Browser::Create(params);
    if (!browser) {
      continue;
    }

    auto* tsm = browser->tab_strip_model();
    for (size_t i = 0; i < window->tabs.size(); ++i) {
      const auto& tab = window->tabs[i];
      if (tab->navigations.empty()) {
        continue;
      }
      chrome::AddRestoredTab(
          browser, tab->navigations,
          /*tab_index=*/i, tab->normalized_navigation_index(),
          tab->extension_app_id,
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
    for (const auto& session_group : window->tab_groups) {
      if (!tsm->group_model() ||
          !tsm->group_model()->ContainsTabGroup(session_group->id)) {
        continue;
      }
      tsm->ChangeTabGroupVisuals(session_group->id, session_group->visual_data);
    }

    int active = std::clamp(window->selected_tab_index, 0,
                            std::max(0, tsm->count() - 1));
    if (active < tsm->count()) {
      tsm->ActivateTabAt(active);
    }
    browser->window()->Show();
  }
}
