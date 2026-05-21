/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/workspaces/workspace_service.h"

#include <memory>
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
#include "brave/browser/workspaces/workspace_session_utils.h"
#include "brave/browser/workspaces/workspace_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/sessions/core/session_id.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace {

std::string ComputeKey(const std::string& name) {
  return absl::StrFormat("%08x", base::PersistentHash(name));
}

}  // namespace

WorkspaceService::WorkspaceService(Profile& profile)
    : profile_(profile),
      workspaces_path_(profile.GetPath().AppendASCII("workspaces")),
      pref_service_(*profile.GetPrefs()),
      io_task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})) {}

WorkspaceService::~WorkspaceService() = default;

std::vector<WorkspaceMetadata> WorkspaceService::ListWorkspaces() const {
  return ListWorkspacesFromDict(
      pref_service_->GetDict(kWorkspacesMetadataPref));
}

void WorkspaceService::SaveWorkspaceMetadata(const WorkspaceMetadata& meta) {
  ScopedDictPrefUpdate updated(*pref_service_, kWorkspacesMetadataPref);
  updated->Set(ComputeKey(meta.name), WorkspaceMetadataToDictEntry(meta));
}

void WorkspaceService::RemoveWorkspaceMetadata(const std::string& name) {
  ScopedDictPrefUpdate updated(*pref_service_, kWorkspacesMetadataPref);
  updated->Remove(ComputeKey(name));
}

base::FilePath WorkspaceService::GetWorkspacePathForName(
    const std::string& name) const {
  return workspaces_path_.AppendASCII(ComputeKey(name));
}

void WorkspaceService::SaveWorkspace(const std::string& name) {
  if (name.empty()) {
    return;
  }

  // Collect session commands on the UI thread, then write to disk on a
  // background task (WriteWorkspaceToDisk does blocking file I/O).
  WorkspaceMetadata workspace{.name = name, .modified_at = base::Time::Now()};
  std::vector<std::unique_ptr<sessions::SessionCommand>> commands =
      GenerateBrowserSessionCommandsForWorkspace(base::to_address(profile_),
                                                 workspace);
  base::FilePath workspace_path = GetWorkspacePathForName(name);

  auto backend = base::MakeRefCounted<sessions::CommandStorageBackend>(
      io_task_runner_, workspace_path, kWorkspaceSessionType,
      /*encryptor=*/std::nullopt);

  // Save metadata optimistically now; roll it back if the write fails.
  // BindPostTask ensures on_error always runs on the UI thread whether it is
  // invoked by AppendCommands or directly by WriteWorkspaceToDisk on a
  // directory-creation failure.
  SaveWorkspaceMetadata(workspace);

  // "Rolling back" is just removing the metadata from the dictionary.
  auto on_error = base::BindPostTask(
      base::SequencedTaskRunner::GetCurrentDefault(),
      base::BindOnce(&WorkspaceService::RemoveWorkspaceMetadata, GetWeakPtr(),
                     name));

  io_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&WriteWorkspaceToDisk, std::move(commands),
                                std::move(workspace_path), std::move(backend),
                                std::move(on_error)));
}

void WorkspaceService::RestoreWorkspace(const std::string& name) {
  base::FilePath path = GetWorkspacePathForName(name);
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

  RestoreBrowserSessionCommandsForWorkspace(base::to_address(profile_),
                                            std::move(commands));
}

base::WeakPtr<WorkspaceService> WorkspaceService::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void WorkspaceService::Shutdown() {
  weak_ptr_factory_.InvalidateWeakPtrs();
}
