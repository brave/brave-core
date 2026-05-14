/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/workspace/workspace_utils.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/time/time.h"
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

constexpr const char kWorkspaceName[] = "name";
constexpr const char kWorkspaceWindowCount[] = "number-of-windows";
constexpr const char kWorkspaceTabCount[] = "number-of-tabs";
constexpr const char kWorkspaceModifiedAt[] = "modified-at";

}  // namespace

std::vector<WorkspaceMetadata> ListWorkspacesFromDict(
    const base::DictValue& dict) {
  std::vector<WorkspaceMetadata> result;
  for (const auto [key, value] : dict) {
    const base::DictValue* entry = value.GetIfDict();
    if (!entry) {
      continue;
    }
    const std::string* name = entry->FindString(kWorkspaceName);
    if (!name || name->empty()) {
      continue;
    }
    std::optional<double> modified_at = entry->FindDouble(kWorkspaceModifiedAt);
    if (!modified_at) {
      continue;
    }
    WorkspaceMetadata info;
    info.name = *name;
    info.number_of_windows = entry->FindInt(kWorkspaceWindowCount).value_or(1);
    info.number_of_tabs = entry->FindInt(kWorkspaceTabCount).value_or(0);
    info.modified_at = base::Time::FromSecondsSinceUnixEpoch(*modified_at);
    result.push_back(std::move(info));
  }
  std::sort(result.begin(), result.end(),
            [](const WorkspaceMetadata& a, const WorkspaceMetadata& b) {
              return a.modified_at > b.modified_at;
            });
  return result;
}

base::DictValue WorkspaceMetadataToDictEntry(const WorkspaceMetadata& meta) {
  base::DictValue entry;
  entry.Set(kWorkspaceName, meta.name);
  entry.Set(kWorkspaceWindowCount, meta.number_of_windows);
  entry.Set(kWorkspaceTabCount, meta.number_of_tabs);
  entry.Set(kWorkspaceModifiedAt, meta.modified_at.InSecondsFSinceUnixEpoch());
  return entry;
}

void WriteWorkspaceToDisk(
    std::vector<std::unique_ptr<sessions::SessionCommand>> commands,
    const base::FilePath& workspace_dir,
    scoped_refptr<sessions::CommandStorageBackend> backend,
    base::OnceClosure on_error) {
  if (!base::CreateDirectory(workspace_dir)) {
    DVLOG(1) << "Failed to create workspace directory: " << workspace_dir;
    std::move(on_error).Run();
    return;
  }
  // AppendCommands posts |on_error| to the backend's callback_task_runner_
  // (the UI thread, set when the backend was constructed) on write failure.
  backend->AppendCommands(std::move(commands), /*truncate=*/true,
                          std::move(on_error));
}

std::vector<std::unique_ptr<sessions::SessionCommand>> ReadWorkspaceFromDisk(
    const base::FilePath& workspace_dir,
    scoped_refptr<sessions::CommandStorageBackend> backend) {
  // Only do file I/O here.  Callers must call RestoreSessionFromCommands() on
  // the UI thread because SessionTab/SessionWindow constructors call
  // SessionID::NewUnique() which is sequence-checked to the UI thread.
  sessions::CommandStorageBackend::ReadCommandsResult result =
      backend->ReadLastSessionCommands();
  if (result.error_reading || result.commands.empty()) {
    DVLOG(1) << "Could not read workspace session from: " << workspace_dir;
    return {};
  }
  return std::move(result.commands);
}

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
