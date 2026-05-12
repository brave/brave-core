/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/workspace/workspace_service.h"

#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/hash/hash.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/task/bind_post_task.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabrestore.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/global_browser_collection.h"
#include "chrome/browser/ui/tabs/tab_group_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/sessions/content/content_serialized_navigation_builder.h"
#include "components/sessions/core/session_id.h"
#include "components/sessions/core/session_service_commands.h"
#include "components/tab_groups/tab_group_visual_data.h"
#include "components/tabs/public/tab_group.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace {

// Keys within each per-workspace dict entry.
constexpr const char kWorkspaceName[] = "name";
constexpr const char kWorkspaceWindowCount[] = "number-of-windows";
constexpr const char kWorkspaceTabCount[] = "number-of-tabs";
constexpr const char kWorkspaceModifiedAt[] = "modified-at";

// Appends session commands for a single browser window to |commands| and
// updates |active_window_id| to this window if it is active (focused), or if
// no active window has been recorded yet. Returns the tab count serialized.
int AppendBrowserSessionCommands(
    SessionID& window_id,
    TabStripModel* tsm,
    gfx::Rect restored_bounds,
    ui::mojom::WindowShowState restored_state,
    std::vector<std::unique_ptr<sessions::SessionCommand>>& commands) {
  if (tsm->count() == 0) {
    return 0;
  }

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

  int tab_count = 0;
  for (int i = 0; i < tsm->count(); ++i) {
    content::WebContents* contents = tsm->GetWebContentsAt(i);
    if (!contents) {
      continue;
    }

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
    tab_count++;
  }

  commands.push_back(sessions::CreateSetSelectedTabInWindowCommand(
      window_id, tsm->active_index()));
  return tab_count;
}

}  // namespace

WorkspaceService::WorkspaceService(PrefService* pref_service,
                                   const base::FilePath profile_path)
    : profile_path_(profile_path), pref_service_(pref_service) {}

WorkspaceService::~WorkspaceService() = default;

std::vector<WorkspaceInfo> WorkspaceService::ListWorkspaces() const {
  std::vector<WorkspaceInfo> result;
  const base::DictValue& all = pref_service_->GetDict(kWorkspacesMetadataPref);

  for (const auto [key, value] : all) {
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
    WorkspaceInfo info;
    info.name = *name;
    info.number_of_windows = entry->FindInt(kWorkspaceWindowCount).value_or(1);
    info.number_of_tabs = entry->FindInt(kWorkspaceTabCount).value_or(0);
    info.modified_at = base::Time::FromSecondsSinceUnixEpoch(*modified_at);
    result.push_back(std::move(info));
  }

  std::sort(result.begin(), result.end(),
            [](const WorkspaceInfo& a, const WorkspaceInfo& b) {
              return a.modified_at > b.modified_at;
            });
  return result;
}

void WorkspaceService::SaveWorkspaceMetadata(const std::string& name,
                                             int window_count,
                                             int tab_count,
                                             base::Time modified_at) {
  base::DictValue updated =
      pref_service_->GetDict(kWorkspacesMetadataPref).Clone();

  base::DictValue entry;
  entry.Set(kWorkspaceName, name);
  entry.Set(kWorkspaceWindowCount, window_count);
  entry.Set(kWorkspaceTabCount, tab_count);
  entry.Set(kWorkspaceModifiedAt, modified_at.InSecondsFSinceUnixEpoch());

  updated.Set(ComputeUniqueKey(name), std::move(entry));
  pref_service_->SetDict(kWorkspacesMetadataPref, std::move(updated));
}

void WorkspaceService::RemoveWorkspaceMetadata(const std::string& name) {
  base::DictValue updated =
      pref_service_->GetDict(kWorkspacesMetadataPref).Clone();
  updated.Remove(ComputeUniqueKey(name));
  pref_service_->SetDict(kWorkspacesMetadataPref, std::move(updated));
}

// static
bool WorkspaceService::DeleteWorkspace(const base::FilePath& workspace_dir) {
  if (!base::PathExists(workspace_dir)) {
    return true;
  }
  return base::DeletePathRecursively(workspace_dir);
}

base::FilePath WorkspaceService::GetWorkspacesDir() const {
  return WorkspacesDir();
}

base::FilePath WorkspaceService::GetWorkspaceDirForName(
    const std::string& name) const {
  return WorkspaceDirForName(name);
}

// static
void WorkspaceService::WriteWorkspaceToDisk(
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

// static
std::vector<std::unique_ptr<sessions::SessionCommand>>
WorkspaceService::ReadWorkspaceFromDisk(
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

void WorkspaceService::SaveWorkspace(Profile* profile,
                                     const std::string& name) {
  if (name.empty()) {
    return;
  }

  // Collect session commands on the UI thread, then write to disk on a
  // background task (WriteWorkspaceToDisk does blocking file I/O).
  std::vector<std::unique_ptr<sessions::SessionCommand>> commands;

  SessionID active_window_id = SessionID::InvalidValue();
  int window_count = 0;
  int tab_count = 0;

  GlobalBrowserCollection::GetInstance()->ForEach(
      [&](BrowserWindowInterface* bwi) {
        if (bwi->GetProfile() != profile ||
            bwi->GetType() != BrowserWindowInterface::Type::TYPE_NORMAL) {
          return true;
        }

        SessionID window_id = SessionID::NewUnique();
        auto* window = bwi->GetWindow();
        int tabs = AppendBrowserSessionCommands(
            window_id, bwi->GetTabStripModel(), window->GetRestoredBounds(),
            window->GetRestoredState(), commands);
        if (tabs == 0) {
          return true;
        }
        // Prefer the focused window; fall back to the first non-empty window.
        if (window->IsActive() ||
            active_window_id == SessionID::InvalidValue()) {
          active_window_id = window_id;
        }
        window_count++;
        tab_count += tabs;
        return true;
      });

  if (tab_count == 0) {
    return;
  }

  if (active_window_id != SessionID::InvalidValue()) {
    commands.push_back(
        sessions::CreateSetActiveWindowCommand(active_window_id));
  }

  base::FilePath workspace_dir = GetWorkspaceDirForName(name);

  // CommandStorageBackend must be constructed on the UI thread because its
  // constructor calls SingleThreadTaskRunner::GetCurrentDefault().  We create
  // a MayBlock SequencedTaskRunner here and pass it as the backend's owning
  // runner, then post the actual file I/O to that same runner.
  auto task_runner = base::ThreadPool::CreateSequencedTaskRunner(
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::BLOCK_SHUTDOWN});
  auto backend = base::MakeRefCounted<sessions::CommandStorageBackend>(
      task_runner, workspace_dir, kWorkspaceSessionType,
      /*encryptor=*/std::nullopt);

  // Save metadata optimistically now; roll it back if the write fails.
  // BindPostTask ensures on_error always runs on the UI thread whether it is
  // invoked by AppendCommands (via callback_task_runner_) or directly by
  // WriteWorkspaceToDisk on a directory-creation failure.
  SaveWorkspaceMetadata(name, window_count, tab_count, base::Time::Now());

  // "Rolling back" is just removing the metadata from the dictionary.
  auto on_error = base::BindPostTask(
      base::SequencedTaskRunner::GetCurrentDefault(),
      base::BindOnce(&WorkspaceService::RemoveWorkspaceMetadata, GetWeakPtr(),
                     name));

  task_runner->PostTask(
      FROM_HERE, base::BindOnce(&WorkspaceService::WriteWorkspaceToDisk,
                                std::move(commands), std::move(workspace_dir),
                                std::move(backend), std::move(on_error)));
}

void WorkspaceService::RestoreWorkspace(Profile* profile,
                                        const std::string& name) {
  base::FilePath path = GetWorkspaceDirForName(name);

  // Construct the backend on the UI thread (requires SingleThreadTaskRunner
  // context), then hand it to the background sequenced runner for file I/O.
  auto task_runner = base::ThreadPool::CreateSequencedTaskRunner(
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  auto backend = base::MakeRefCounted<sessions::CommandStorageBackend>(
      task_runner, path, kWorkspaceSessionType,
      /*encryptor=*/std::nullopt);

  task_runner->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&WorkspaceService::ReadWorkspaceFromDisk, std::move(path),
                     std::move(backend)),
      base::BindOnce(&WorkspaceService::DoRestoreWorkspace, GetWeakPtr(),
                     profile->GetWeakPtr()));
}

void WorkspaceService::ShowSaveWorkspaceDialog(Profile* profile) {
  // TODO(https://github.com/brave/brave-browser/issues/55108)
  SaveWorkspace(profile, "example-workspace");
}

void WorkspaceService::ShowOpenWorkspaceDialog(Profile* profile) {
  // TODO(https://github.com/brave/brave-browser/issues/55108)
  RestoreWorkspace(profile, "example-workspace");
}

void WorkspaceService::DoRestoreWorkspace(
    base::WeakPtr<Profile> profile,
    std::vector<std::unique_ptr<sessions::SessionCommand>> commands) {
  if (commands.empty()) {
    DVLOG(1) << "Could not load workspace: no commands";
    return;
  }

  if (!profile) {
    DVLOG(1) << "Could not load workspace: profile is null";
    return;
  }

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

    if (Browser::GetCreationStatusForProfile(profile.get()) !=
        Browser::CreationStatus::kOk) {
      continue;
    }
    Browser::CreateParams params(Browser::TYPE_NORMAL, profile.get(), false);
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

base::WeakPtr<WorkspaceService> WorkspaceService::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void WorkspaceService::Shutdown() {
  weak_ptr_factory_.InvalidateWeakPtrs();
}

// static
std::string WorkspaceService::SanitizeName(const std::string& name) {
  std::string sanitized;
  sanitized.reserve(name.size());
  for (char c : name) {
    if (base::IsAsciiAlphaNumeric(c) || c == '-' || c == '_') {
      sanitized += base::ToLowerASCII(c);
    } else if (c == ' ') {
      sanitized += '-';
    }
    if (sanitized.size() >= 64) {
      break;
    }
  }
  if (sanitized.empty()) {
    // Use a stable hash of the original so two distinct inputs that both
    // produce an empty sanitized form (e.g. "" and "!!!") still get different
    // base keys, preventing silent collisions at a fixed fallback string.
    return absl::StrFormat("workspace-%08x", base::PersistentHash(name));
  }
  return sanitized;
}

base::FilePath WorkspaceService::WorkspacesDir() const {
  return profile_path_.AppendASCII("workspaces");
}

std::string WorkspaceService::ComputeUniqueKey(const std::string& name) const {
  const base::DictValue& all = pref_service_->GetDict(kWorkspacesMetadataPref);
  const std::string base_key = SanitizeName(name);

  // Returns true if |key| is free to use for |name|: either the slot is empty
  // or it already belongs to this display name (update / re-save case).
  auto is_available = [&](const std::string& key) {
    const base::DictValue* entry = all.FindDict(key);
    if (!entry) {
      return true;
    }
    const std::string* stored = entry->FindString(kWorkspaceName);
    return stored && *stored == name;
  };

  if (is_available(base_key)) {
    return base_key;
  }
  // Append a stable hash of the display name so the key is deterministic
  // without a serial number.  Two names that collide on SanitizeName are
  // overwhelmingly unlikely to also collide on PersistentHash.
  return absl::StrFormat("%s-%08x", base_key.c_str(),
                         base::PersistentHash(name));
}

base::FilePath WorkspaceService::WorkspaceDirForName(
    const std::string& name) const {
  const std::string key = ComputeUniqueKey(name);
  // SanitizeName's allow-list ([a-z0-9-_] only) guarantees no path separators
  // or dot sequences can survive into the key.  Assert as defence-in-depth.
  DCHECK(!key.empty() && key.find_first_of("/\\.") == std::string::npos);
  return WorkspacesDir().AppendASCII(key);
}
