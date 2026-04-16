/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WORKSPACE_BRAVE_WORKSPACE_SERVICE_H_
#define BRAVE_BROWSER_WORKSPACE_BRAVE_WORKSPACE_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "brave/browser/workspace/brave_workspace.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/sessions/core/command_storage_backend.h"
#include "components/sessions/core/command_storage_manager.h"
#include "components/sessions/core/session_command.h"
#include "components/sessions/core/session_types.h"

class Profile;

// Per-profile service that manages saving and restoring named workspaces.
//
// Each workspace is stored in its own subdirectory under
//   {profile_dir}/workspaces/{sanitized_name}/
//
// Inside that subdirectory:
//   info.json            — workspace display name (JSON, tiny)
//   Sessions/Session_*   — full browser state in Chromium's session-command
//                          binary format (handled by CommandStorageBackend)
//
// This service handles only file I/O and serialization.  Actual browser-state
// iteration and restoration are handled by the browser-layer commands
// brave::SaveWorkspace() / brave::RestoreWorkspace() in browser_commands.cc.
//
// Threading note: CommandStorageBackend must be constructed on the UI thread
// because its constructor calls SingleThreadTaskRunner::GetCurrentDefault().
// Callers must therefore create the backend on the UI thread (passing a
// MayBlock SequencedTaskRunner as the owning runner) and then invoke the
// blocking I/O helpers on that same sequenced runner.
class BraveWorkspaceService : public KeyedService {
 public:
  // Session type used for all workspace files.  Exposed so callers can
  // construct a CommandStorageBackend on the UI thread before posting I/O
  // work to a background sequenced runner.
  static constexpr sessions::CommandStorageManager::SessionType
      kWorkspaceSessionType =
          sessions::CommandStorageManager::SessionType::kSessionRestore;

  explicit BraveWorkspaceService(Profile* profile);
  ~BraveWorkspaceService() override;

  BraveWorkspaceService(const BraveWorkspaceService&) = delete;
  BraveWorkspaceService& operator=(const BraveWorkspaceService&) = delete;

  // Returns summary information for all saved workspaces, sorted by creation
  // time (most-recent first).
  std::vector<WorkspaceInfo> ListWorkspaces() const;

  // Deletes the workspace directory for |name|.  Returns true on success.
  bool DeleteWorkspace(const std::string& name);

  // Returns the directory that contains all workspace subdirectories.
  base::FilePath GetWorkspacesDir() const;

  // Returns the per-workspace subdirectory for |name|.
  base::FilePath GetWorkspaceDirForName(const std::string& name) const;

  // ---- Blocking I/O helpers (run on a thread-pool sequenced task) ----------
  //
  // |backend| must be constructed on the UI thread before posting these tasks.

  // Writes |name| to info.json and |commands| to a Chromium session-command
  // binary file via |backend|.  Returns true on success.
  static bool WriteWorkspaceToDisk(
      const std::string& name,
      std::vector<std::unique_ptr<sessions::SessionCommand>> commands,
      const base::FilePath& workspace_dir,
      scoped_refptr<sessions::CommandStorageBackend> backend);

  // Reads the session-command binary via |backend| and returns the raw
  // commands.  Only does file I/O — callers must deserialize the commands into
  // SessionWindow objects on the UI thread (SessionID::NewUnique() is
  // sequence-checked to the UI thread).  Returns an empty vector on error.
  static std::vector<std::unique_ptr<sessions::SessionCommand>>
  ReadWorkspaceFromDisk(const base::FilePath& workspace_dir,
                        scoped_refptr<sessions::CommandStorageBackend> backend);

  // Scans |workspaces_dir| for workspace subdirectories and returns summary
  // info for each, sorted most-recent first.
  static std::vector<WorkspaceInfo> ListWorkspacesInDir(
      const base::FilePath& workspaces_dir);

 private:
  static std::string SanitizeName(const std::string& name);

  base::FilePath WorkspacesDir() const;
  base::FilePath WorkspaceDirForName(const std::string& name) const;

  raw_ptr<Profile> profile_;
};

#endif  // BRAVE_BROWSER_WORKSPACE_BRAVE_WORKSPACE_SERVICE_H_
