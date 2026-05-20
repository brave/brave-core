/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WORKSPACES_WORKSPACE_SERVICE_H_
#define BRAVE_BROWSER_WORKSPACES_WORKSPACE_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/raw_ref.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/browser/workspaces/pref_names.h"
#include "brave/browser/workspaces/workspace_metadata.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/sessions/core/session_command.h"

class PrefService;
class Profile;

// Per-profile service that manages saving and restoring named workspaces.
//
// Each workspace is stored in its own subdirectory under
//   {profile_dir}/workspaces/{hashed_name}/
//
// Inside that subdirectory:
//   Sessions/Session_*   — full browser state in Chromium's session-command
//                          binary format (handled by CommandStorageBackend)
//
// Workspace metadata (display name, window/tab counts, creation time) is
// stored in a profile preference rather than a per-directory info.json file.
//
// Threading note: All workspace file I/O runs on |io_task_runner_|, a
// MayBlock SequencedTaskRunner created at construction time.  Backends are
// constructed on the UI thread with |io_task_runner_| as their owning
// sequence, then all blocking I/O tasks are posted to that same runner.
class WorkspaceService : public KeyedService {
 public:
  explicit WorkspaceService(Profile& profile);
  ~WorkspaceService() override;

  WorkspaceService(const WorkspaceService&) = delete;
  WorkspaceService& operator=(const WorkspaceService&) = delete;

  // Returns summary information for all saved workspaces, sorted by time
  // modified (most-recent first).  Reads from the profile preference; no disk
  // I/O.
  std::vector<WorkspaceMetadata> ListWorkspaces() const;

  // Writes workspace metadata into the profile preference.  Called on the UI
  // thread after a successful WriteWorkspaceToDisk background task.
  void SaveWorkspaceMetadata(const WorkspaceMetadata& meta);

  // Removes the workspace metadata entry from the profile preference.
  void RemoveWorkspaceMetadata(const std::string& name);

  // Returns the directory that contains all workspace subdirectories.
  const base::FilePath& GetWorkspacesPath() const { return workspaces_path_; }

  // Returns the per-workspace subdirectory for |name|.
  base::FilePath GetWorkspacePathForName(const std::string& name) const;

  // ---- Browser-state commands (UI thread only) ----------------------------

  // Serializes all open windows/tabs for this profile and writes them to disk
  // under the given workspace name.
  void SaveWorkspace(const std::string& name);

  // Reads the named workspace from disk and opens its windows/tabs.
  void RestoreWorkspace(const std::string& name);

  // Placeholder entry points for the save/open dialogs.
  void ShowSaveWorkspaceDialog();
  void ShowOpenWorkspaceDialog();

  base::WeakPtr<WorkspaceService> GetWeakPtr();

  // KeyedService:
  void Shutdown() override;

 private:
  // Called on the UI thread with the commands read from disk by
  // RestoreWorkspace.
  void DoRestoreWorkspace(
      std::vector<std::unique_ptr<sessions::SessionCommand>> commands);

  raw_ref<Profile> profile_;
  const base::FilePath workspaces_path_;
  raw_ref<PrefService> pref_service_;
  scoped_refptr<base::SequencedTaskRunner> io_task_runner_;

  base::WeakPtrFactory<WorkspaceService> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_WORKSPACES_WORKSPACE_SERVICE_H_
