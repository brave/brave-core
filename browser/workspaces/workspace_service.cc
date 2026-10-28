/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/workspaces/workspace_service.h"

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/task/bind_post_task.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "base/uuid.h"
#include "brave/browser/workspaces/workspace_session_utils.h"
#include "brave/browser/workspaces/workspace_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/profile_browser_collection.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

WorkspaceService::WorkspaceService(Profile& profile)
    : profile_(profile),
      workspaces_path_(profile.GetPath().AppendASCII("workspaces")),
      pref_service_(*profile.GetPrefs()),
      io_task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})) {
  // Null in unit tests, which construct the service without browser windows.
  if (auto* collection =
          ProfileBrowserCollection::GetForProfile(base::to_address(profile_))) {
    browser_collection_observation_.Observe(collection);
  }
}

WorkspaceService::~WorkspaceService() = default;

std::vector<WorkspaceMetadata> WorkspaceService::ListWorkspaces() const {
  return ListWorkspacesFromDict(
      pref_service_->GetDict(kWorkspacesMetadataPref));
}

std::optional<WorkspaceMetadata> WorkspaceService::GetWorkspace(
    const base::Uuid& id) const {
  for (auto& workspace : ListWorkspaces()) {
    if (workspace.id == id) {
      return workspace;
    }
  }
  return std::nullopt;
}

WorkspaceMetadata WorkspaceService::CreateWorkspace(const std::string& name) {
  WorkspaceMetadata workspace{.id = base::Uuid::GenerateRandomV4(),
                              .name = name,
                              .modified_at = base::Time::Now(),
                              .number_of_tabs = 0};
  SaveWorkspaceMetadata(workspace);
  return workspace;
}

void WorkspaceService::RenameWorkspace(const base::Uuid& id,
                                       const std::string& name) {
  if (name.empty()) {
    return;
  }
  std::optional<WorkspaceMetadata> workspace = GetWorkspace(id);
  if (!workspace) {
    return;
  }
  // modified_at tracks tab changes, so renaming does not reorder the list.
  workspace->name = name;
  SaveWorkspaceMetadata(*workspace);
}

void WorkspaceService::SaveWorkspaceMetadata(const WorkspaceMetadata& meta) {
  CHECK(meta.id.is_valid());
  ScopedDictPrefUpdate updated(*pref_service_, kWorkspacesMetadataPref);
  updated->Set(meta.id.AsLowercaseString(), WorkspaceMetadataToDictEntry(meta));
}

void WorkspaceService::RemoveWorkspaceMetadata(const base::Uuid& id) {
  ScopedDictPrefUpdate updated(*pref_service_, kWorkspacesMetadataPref);
  updated->Remove(id.AsLowercaseString());
}

void WorkspaceService::DeleteWorkspace(const base::Uuid& id) {
  RemoveWorkspaceMetadata(id);
  workspace_to_window_.erase(id);
  io_task_runner_->PostTask(FROM_HERE, base::BindOnce(
                                           [](base::FilePath dir) {
                                             if (base::PathExists(dir)) {
                                               base::DeletePathRecursively(dir);
                                             }
                                           },
                                           GetWorkspacePathForId(id)));
}

base::FilePath WorkspaceService::GetWorkspacePathForId(
    const base::Uuid& id) const {
  return workspaces_path_.AppendASCII(id.AsLowercaseString());
}

std::optional<base::Uuid> WorkspaceService::GetWorkspaceForWindow(
    SessionID window_id) const {
  for (const auto& [id, host] : workspace_to_window_) {
    if (host == window_id) {
      return id;
    }
  }
  return std::nullopt;
}

std::optional<SessionID> WorkspaceService::GetWindowForWorkspace(
    const base::Uuid& id) const {
  auto it = workspace_to_window_.find(id);
  if (it == workspace_to_window_.end()) {
    return std::nullopt;
  }
  return it->second;
}

void WorkspaceService::UnbindWindow(SessionID window_id) {
  std::erase_if(workspace_to_window_, [window_id](const auto& entry) {
    return entry.second == window_id;
  });
}

void WorkspaceService::SaveWorkspaceFromWindow(const base::Uuid& id,
                                               BrowserWindowInterface* window) {
  std::optional<WorkspaceMetadata> previous = GetWorkspace(id);
  if (!previous) {
    return;
  }

  // Collect session commands on the UI thread, then write to disk on a
  // background task (WriteWorkspaceToDisk does blocking file I/O).
  WorkspaceMetadata workspace = *previous;
  workspace.modified_at = base::Time::Now();
  std::vector<std::unique_ptr<sessions::SessionCommand>> commands =
      SerializeWindowForWorkspace(window, workspace);
  base::FilePath workspace_path = GetWorkspacePathForId(id);

  auto backend = base::MakeRefCounted<sessions::CommandStorageBackend>(
      io_task_runner_, workspace_path, kWorkspaceSessionType,
      /*encryptor=*/nullptr);

  // Record the new tab count optimistically, then put the previous entry back
  // if the write fails so the list never advertises tabs that were not
  // persisted.  BindPostTask ensures on_error always runs on the UI thread
  // whether it is invoked by AppendCommands or directly by
  // WriteWorkspaceToDisk on a directory-creation failure.
  SaveWorkspaceMetadata(workspace);

  auto on_error = base::BindPostTask(
      base::SequencedTaskRunner::GetCurrentDefault(),
      base::BindOnce(&WorkspaceService::SaveWorkspaceMetadata, GetWeakPtr(),
                     *previous));

  io_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&WriteWorkspaceToDisk, std::move(commands),
                                std::move(workspace_path), std::move(backend),
                                std::move(on_error)));
}

void WorkspaceService::LoadWorkspaceIntoWindow(const base::Uuid& id,
                                               BrowserWindowInterface* window) {
  if (!GetWorkspace(id)) {
    return;
  }

  const SessionID window_id = window->GetSessionID();
  std::optional<SessionID> host = GetWindowForWorkspace(id);
  if (host && *host != window_id) {
    DVLOG(1) << "Workspace is already loaded in another window";
    return;
  }

  // A window hosts at most one workspace.
  UnbindWindow(window_id);
  workspace_to_window_.insert_or_assign(id, window_id);

  base::FilePath path = GetWorkspacePathForId(id);
  auto backend = base::MakeRefCounted<sessions::CommandStorageBackend>(
      io_task_runner_, path, kWorkspaceSessionType,
      /*encryptor=*/nullptr);

  io_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&ReadWorkspaceFromDisk, std::move(path),
                     std::move(backend)),
      base::BindOnce(&WorkspaceService::DoLoadWorkspaceIntoWindow, GetWeakPtr(),
                     id, window_id));
}

void WorkspaceService::DoLoadWorkspaceIntoWindow(
    base::Uuid id,
    SessionID window_id,
    std::vector<std::unique_ptr<sessions::SessionCommand>> commands) {
  BrowserWindowInterface* window =
      BrowserWindowInterface::FromSessionID(window_id);
  if (!window) {
    workspace_to_window_.erase(id);
    return;
  }

  // A workspace with no tabs yet has nothing on disk; the binding still holds.
  if (commands.empty()) {
    return;
  }

  RestoreWorkspaceIntoWindow(window, std::move(commands));
}

base::WeakPtr<WorkspaceService> WorkspaceService::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void WorkspaceService::Shutdown() {
  browser_collection_observation_.Reset();
  weak_ptr_factory_.InvalidateWeakPtrs();
}

void WorkspaceService::OnBrowserClosed(BrowserWindowInterface* browser) {
  // Releasing the binding keeps a closed window from holding a workspace
  // hostage. Saving the window's tabs first will hook in here once the
  // load/unload mechanism is settled.
  UnbindWindow(browser->GetSessionID());
}
