/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WORKSPACES_WORKSPACE_SERVICE_H_
#define BRAVE_BROWSER_WORKSPACES_WORKSPACE_SERVICE_H_

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/raw_ref.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "base/task/sequenced_task_runner.h"
#include "base/uuid.h"
#include "brave/browser/workspaces/pref_names.h"
#include "brave/browser/workspaces/workspace_metadata.h"
#include "chrome/browser/ui/browser_window/public/browser_collection_observer.h"
#include "chrome/browser/ui/browser_window/public/profile_browser_collection.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/sessions/core/session_command.h"
#include "components/sessions/core/session_id.h"

class BrowserWindowInterface;
class PrefService;
class Profile;

// Per-profile service that manages named workspaces.
//
// A workspace owns the tabs of a single browser window, and may be loaded into
// at most one window at a time.  Workspaces themselves belong to the profile,
// so every window of a profile can see the same list.
//
// Each workspace is stored in its own subdirectory under
//   {profile_dir}/workspaces/{uuid}/
//
// Inside that subdirectory:
//   Sessions/Session_*   — tab state in Chromium's session-command binary
//                          format (handled by CommandStorageBackend)
//
// Workspace metadata (display name, tab count, time modified) is stored in a
// profile preference, keyed by the same UUID.  Identity is a UUID rather than
// a hash of the name so that a workspace can be renamed without moving
// storage, and so a future sync implementation has a stable key.
//
// Threading note: All workspace file I/O runs on |io_task_runner_|, a
// MayBlock SequencedTaskRunner created at construction time.  Backends are
// constructed on the UI thread with |io_task_runner_| as their owning
// sequence, then all blocking I/O tasks are posted to that same runner.
class WorkspaceService : public KeyedService, public BrowserCollectionObserver {
 public:
  explicit WorkspaceService(Profile& profile);
  ~WorkspaceService() override;

  WorkspaceService(const WorkspaceService&) = delete;
  WorkspaceService& operator=(const WorkspaceService&) = delete;

  // ---- Metadata (preference-backed; no disk I/O) --------------------------

  // Returns summary information for all workspaces in this profile, sorted by
  // time modified (most-recent first).
  std::vector<WorkspaceMetadata> ListWorkspaces() const;

  // Returns the workspace with |id|, or nullopt if there is no such workspace.
  std::optional<WorkspaceMetadata> GetWorkspace(const base::Uuid& id) const;

  // Registers a new, empty workspace named |name| and returns it.  Nothing is
  // written to disk until tabs are saved into it, so an empty workspace exists
  // purely as metadata.
  WorkspaceMetadata CreateWorkspace(const std::string& name);

  // Changes a workspace's display name.  Storage is keyed by UUID, so nothing
  // moves on disk.  Does nothing if |id| is unknown.
  void RenameWorkspace(const base::Uuid& id, const std::string& name);

  // Writes workspace metadata into the profile preference.
  void SaveWorkspaceMetadata(const WorkspaceMetadata& meta);

  // Removes the workspace metadata entry from the profile preference.
  void RemoveWorkspaceMetadata(const base::Uuid& id);

  // Removes workspace metadata from prefs, releases any window binding, and
  // deletes its on-disk directory.  The pref update runs on the UI thread; the
  // directory deletion is posted to the internal I/O task runner.
  void DeleteWorkspace(const base::Uuid& id);

  // ---- Paths --------------------------------------------------------------

  // Returns the directory that contains all workspace subdirectories.
  const base::FilePath& GetWorkspacesPath() const { return workspaces_path_; }

  // Returns the per-workspace subdirectory for |id|.
  base::FilePath GetWorkspacePathForId(const base::Uuid& id) const;

  // ---- Window binding -----------------------------------------------------
  //
  // A workspace may be loaded in at most one window at a time.  Bindings are
  // dropped when the hosting window closes.

  // Returns the workspace currently loaded in |window_id|, or nullopt if that
  // window is not hosting one.
  std::optional<base::Uuid> GetWorkspaceForWindow(SessionID window_id) const;

  // Returns the window currently hosting |id|, or nullopt if it is not loaded.
  std::optional<SessionID> GetWindowForWorkspace(const base::Uuid& id) const;

  // Releases |window_id|'s binding, if it has one.
  void UnbindWindow(SessionID window_id);

  // ---- Browser state (UI thread only) -------------------------------------

  // Serializes |window|'s tabs and writes them to disk as workspace |id|,
  // updating its tab count and modified time.  Does nothing if |id| is
  // unknown.
  void SaveWorkspaceFromWindow(const base::Uuid& id,
                               BrowserWindowInterface* window);

  // Reads workspace |id| from disk, adds its tabs to |window|, and binds the
  // two together.  Tabs already open in |window| are left alone — callers
  // decide what becomes of them.  Does nothing if |id| is already loaded in a
  // different window.
  void LoadWorkspaceIntoWindow(const base::Uuid& id,
                               BrowserWindowInterface* window);

  base::WeakPtr<WorkspaceService> GetWeakPtr();

  // KeyedService:
  void Shutdown() override;

  // BrowserCollectionObserver:
  void OnBrowserClosed(BrowserWindowInterface* browser) override;

 private:
  // Called on the UI thread with the commands read from disk by
  // LoadWorkspaceIntoWindow.  |window_id| identifies the destination window,
  // which may have closed while the read was in flight.
  void DoLoadWorkspaceIntoWindow(
      base::Uuid id,
      SessionID window_id,
      std::vector<std::unique_ptr<sessions::SessionCommand>> commands);

  raw_ref<Profile> profile_;
  const base::FilePath workspaces_path_;
  raw_ref<PrefService> pref_service_;
  scoped_refptr<base::SequencedTaskRunner> io_task_runner_;

  // Which window hosts each loaded workspace.  Small enough that the reverse
  // lookup is a linear scan rather than a second map to keep in sync.
  std::map<base::Uuid, SessionID> workspace_to_window_;

  base::ScopedObservation<ProfileBrowserCollection, BrowserCollectionObserver>
      browser_collection_observation_{this};

  base::WeakPtrFactory<WorkspaceService> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_WORKSPACES_WORKSPACE_SERVICE_H_
