/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WORKSPACE_WORKSPACE_SERVICE_H_
#define BRAVE_BROWSER_WORKSPACE_WORKSPACE_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "brave/browser/workspace/workspace.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/sessions/core/command_storage_backend.h"
#include "components/sessions/core/command_storage_manager.h"
#include "components/sessions/core/session_command.h"

class PrefService;
class Profile;

// Profile preference key — stores a dict keyed by sanitized workspace name.
inline constexpr char kWorkspacesMetadataPref[] = "brave.workspaces";

// Per-profile service that manages saving and restoring named workspaces.
//
// Each workspace is stored in its own subdirectory under
//   {profile_dir}/workspaces/{sanitized_name}/
//
// Inside that subdirectory:
//   Sessions/Session_*   — full browser state in Chromium's session-command
//                          binary format (handled by CommandStorageBackend)
//
// Workspace metadata (display name, window/tab counts, creation time) is
// stored in a profile preference rather than a per-directory info.json file.
//
// Threading note: CommandStorageBackend must be constructed on the UI thread
// because its constructor calls SingleThreadTaskRunner::GetCurrentDefault().
// Callers must therefore create the backend on the UI thread (passing a
// MayBlock SequencedTaskRunner as the owning runner) and then invoke the
// blocking I/O helpers on that same sequenced runner.
class WorkspaceService : public KeyedService {
 public:
  // Session type used for all workspace files.  Exposed so callers can
  // construct a CommandStorageBackend on the UI thread before posting I/O
  // work to a background sequenced runner.
  static constexpr sessions::CommandStorageManager::SessionType
      kWorkspaceSessionType =
          sessions::CommandStorageManager::SessionType::kSessionRestore;

  explicit WorkspaceService(PrefService* pref_service,
                            const base::FilePath profile_path);
  ~WorkspaceService() override;

  WorkspaceService(const WorkspaceService&) = delete;
  WorkspaceService& operator=(const WorkspaceService&) = delete;

  // Returns summary information for all saved workspaces, sorted by creation
  // time (most-recent first).  Reads from the profile preference; no disk I/O.
  std::vector<WorkspaceInfo> ListWorkspaces() const;

  // Writes workspace metadata into the profile preference.  Called on the UI
  // thread after a successful WriteWorkspaceToDisk background task.
  void SaveWorkspaceMetadata(const std::string& name,
                             int window_count,
                             int tab_count,
                             base::Time created_at);

  // Removes the workspace metadata entry from the profile preference.
  void RemoveWorkspaceMetadata(const std::string& name);

  // Deletes |workspace_dir|.  Returns true on success.  Runs on a background
  // thread; the caller must resolve the path on the UI thread first via
  // GetWorkspaceDirForName().
  static bool DeleteWorkspace(const base::FilePath& workspace_dir);

  // Returns the directory that contains all workspace subdirectories.
  base::FilePath GetWorkspacesDir() const;

  // Returns the per-workspace subdirectory for |name|.
  base::FilePath GetWorkspaceDirForName(const std::string& name) const;

  // ---- Blocking I/O helpers (run on a thread-pool sequenced task) ----------
  //
  // |backend| must be constructed on the UI thread before posting these tasks.

  // Writes |commands| to a Chromium session-command binary file via |backend|.
  // Both callbacks are posted to the UI thread: |on_error| (via
  // AppendCommands' callback_task_runner_) if the write fails, |on_success|
  // unconditionally after AppendCommands returns.  Because |on_error| is
  // enqueued first, the UI thread always runs it before |on_success|.
  static void WriteWorkspaceToDisk(
      std::vector<std::unique_ptr<sessions::SessionCommand>> commands,
      const base::FilePath& workspace_dir,
      scoped_refptr<sessions::CommandStorageBackend> backend,
      base::OnceClosure on_success,
      base::OnceClosure on_error);

  // Reads the session-command binary via |backend| and returns the raw
  // commands.  Only does file I/O — callers must deserialize the commands into
  // SessionWindow objects on the UI thread (SessionID::NewUnique() is
  // sequence-checked to the UI thread).  Returns an empty vector on error.
  static std::vector<std::unique_ptr<sessions::SessionCommand>>
  ReadWorkspaceFromDisk(const base::FilePath& workspace_dir,
                        scoped_refptr<sessions::CommandStorageBackend> backend);

  // ---- Browser-state commands (UI thread only) ----------------------------

  // Serializes all open windows/tabs for this profile and writes them to disk
  // under the given workspace name.
  void SaveWorkspace(Profile* profile, const std::string& name);

  // Reads the named workspace from disk and opens its windows/tabs.
  void RestoreWorkspace(Profile* profile, const std::string& name);

  // Placeholder entry points for the save/open dialogs.
  void ShowSaveWorkspaceDialog(Profile* profile);
  void ShowOpenWorkspaceDialog(Profile* profile);

  base::WeakPtr<WorkspaceService> GetWeakPtr();

  // KeyedService:
  void Shutdown() override;

 private:
  static std::string SanitizeName(const std::string& name);

  // Returns the pref key for |name|.  If |name| already has a saved entry,
  // returns its existing key.  For a new name, returns SanitizeName(name) if
  // that key is free, or SanitizeName(name) + "-" + PersistentHash(name) if
  // another display name has claimed the base key.  Must be called on the UI
  // thread because it reads from the profile preference.
  std::string ComputeUniqueKey(const std::string& name) const;

  base::FilePath WorkspacesDir() const;
  base::FilePath WorkspaceDirForName(const std::string& name) const;

  // Called on the UI thread with the commands read from disk by
  // RestoreWorkspace.
  void DoRestoreWorkspace(
      base::WeakPtr<Profile> profile,
      std::vector<std::unique_ptr<sessions::SessionCommand>> commands);

  const base::FilePath profile_path_;
  raw_ptr<PrefService> pref_service_;

  base::WeakPtrFactory<WorkspaceService> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_WORKSPACE_WORKSPACE_SERVICE_H_
