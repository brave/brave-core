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

#include "base/functional/bind.h"
#include "base/hash/hash.h"
#include "base/logging.h"
#include "base/task/bind_post_task.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/browser/workspace/workspace_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabrestore.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/global_browser_collection.h"
#include "chrome/browser/ui/tabs/tab_group_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/prefs/pref_service.h"
#include "components/sessions/core/session_id.h"
#include "components/sessions/core/session_service_commands.h"
#include "components/tab_groups/tab_group_visual_data.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace {

std::string ComputeKey(const std::string& name) {
  return absl::StrFormat("%08x", base::PersistentHash(name));
}

}  // namespace

WorkspaceService::WorkspaceService(Profile* profile)
    : profile_(profile),
      workspaces_path_(profile->GetPath().AppendASCII("workspaces")),
      pref_service_(profile->GetPrefs()),
      io_task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})) {}

WorkspaceService::~WorkspaceService() = default;

std::vector<WorkspaceMetadata> WorkspaceService::ListWorkspaces() const {
  return ListWorkspacesFromDict(
      pref_service_->GetDict(kWorkspacesMetadataPref));
}

void WorkspaceService::SaveWorkspaceMetadata(const WorkspaceMetadata& meta) {
  base::DictValue updated =
      pref_service_->GetDict(kWorkspacesMetadataPref).Clone();
  updated.Set(ComputeKey(meta.name), WorkspaceMetadataToDictEntry(meta));
  pref_service_->SetDict(kWorkspacesMetadataPref, std::move(updated));
}

void WorkspaceService::RemoveWorkspaceMetadata(const std::string& name) {
  base::DictValue updated =
      pref_service_->GetDict(kWorkspacesMetadataPref).Clone();
  updated.Remove(ComputeKey(name));
  pref_service_->SetDict(kWorkspacesMetadataPref, std::move(updated));
}

base::FilePath WorkspaceService::GetWorkspacesDir() const {
  return workspaces_path_;
}

base::FilePath WorkspaceService::GetWorkspaceDirForName(
    const std::string& name) const {
  return workspaces_path_.AppendASCII(ComputeKey(name));
}

void WorkspaceService::SaveWorkspace(const std::string& name) {
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
        if (bwi->GetProfile() != profile_ ||
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

  auto backend = base::MakeRefCounted<sessions::CommandStorageBackend>(
      io_task_runner_, workspace_dir, kWorkspaceSessionType,
      /*encryptor=*/std::nullopt);

  // Save metadata optimistically now; roll it back if the write fails.
  // BindPostTask ensures on_error always runs on the UI thread whether it is
  // invoked by AppendCommands or directly by WriteWorkspaceToDisk on a
  // directory-creation failure.
  WorkspaceMetadata meta;
  meta.name = name;
  meta.number_of_windows = window_count;
  meta.number_of_tabs = tab_count;
  meta.modified_at = base::Time::Now();
  SaveWorkspaceMetadata(meta);

  // "Rolling back" is just removing the metadata from the dictionary.
  auto on_error = base::BindPostTask(
      base::SequencedTaskRunner::GetCurrentDefault(),
      base::BindOnce(&WorkspaceService::RemoveWorkspaceMetadata, GetWeakPtr(),
                     name));

  io_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&WriteWorkspaceToDisk, std::move(commands),
                                std::move(workspace_dir), std::move(backend),
                                std::move(on_error)));
}

void WorkspaceService::RestoreWorkspace(const std::string& name) {
  base::FilePath path = GetWorkspaceDirForName(name);
  auto backend = base::MakeRefCounted<sessions::CommandStorageBackend>(
      io_task_runner_, path, kWorkspaceSessionType,
      /*encryptor=*/std::nullopt);

  io_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&ReadWorkspaceFromDisk, std::move(path),
                     std::move(backend)),
      base::BindOnce(&WorkspaceService::DoRestoreWorkspace, GetWeakPtr()));
}

void WorkspaceService::ShowSaveWorkspaceDialog() {
  // TODO(https://github.com/brave/brave-browser/issues/55108)
  SaveWorkspace("example-workspace");
}

void WorkspaceService::ShowOpenWorkspaceDialog() {
  // TODO(https://github.com/brave/brave-browser/issues/55108)
  RestoreWorkspace("example-workspace");
}

void WorkspaceService::DoRestoreWorkspace(
    std::vector<std::unique_ptr<sessions::SessionCommand>> commands) {
  if (commands.empty()) {
    DVLOG(1) << "Could not load workspace: no commands";
    return;
  }

  if (!profile_) {
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

    if (Browser::GetCreationStatusForProfile(profile_) !=
        Browser::CreationStatus::kOk) {
      continue;
    }
    Browser::CreateParams params(Browser::TYPE_NORMAL, profile_, false);
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
