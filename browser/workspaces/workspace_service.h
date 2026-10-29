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

#include "base/callback_list.h"
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
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
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
class WorkspaceService : public KeyedService,
                         public BrowserCollectionObserver,
                         public TabStripModelObserver {
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

  // ---- Windows ------------------------------------------------------------
  //
  // A workspace owns a browser window outright.  Switching does not move tabs:
  // the outgoing window is hidden and the incoming one is shown, so no
  // WebContents is ever destroyed and scroll position, media playback, form
  // state and running scripts all survive untouched.
  //
  // A window with no workspace bound to it is an "unmanaged" window: the
  // one a person starts in.  Chromium's own session restore already persists
  // its tabs, so this service never writes them to disk.  It is hidden and
  // shown like any other window, which is all that is needed to switch back to
  // it.

  // Returns the workspace shown in |window_id|, or nullopt if it is an
  // unmanaged window.
  std::optional<base::Uuid> GetWorkspaceForWindow(SessionID window_id) const;

  // Returns the window owned by |id|, or nullopt if it has not been opened in
  // this session yet.
  std::optional<SessionID> GetWindowForWorkspace(const base::Uuid& id) const;

  // Returns the display name of the workspace shown in |window_id|, or an
  // empty string when that window is unmanaged.
  std::string GetWorkspaceNameForWindow(SessionID window_id) const;

  // Notifies when the workspace shown in some window changes, or is renamed.
  // Subscribers re-read whichever window they belong to.
  base::CallbackListSubscription RegisterWorkspacesChanged(
      base::RepeatingClosure callback);

  // True when |id| is on screen in some window other than |window_id|.
  //
  // A workspace that is open but hidden is not held by anyone: switching to it
  // simply brings its window forward, so it stays pickable. Only a workspace
  // somebody is actually looking at is out of reach.
  bool IsWorkspaceOnScreenElsewhere(const base::Uuid& id,
                                    SessionID window_id) const;

  // Returns the unmanaged window |window_id| was opened from, or nullopt if
  // |window_id| is itself an unmanaged window.
  std::optional<SessionID> GetUnmanagedWindowFor(SessionID window_id) const;

  // Releases |window_id|'s binding, if it has one.
  void UnbindWindow(SessionID window_id);

  // ---- Browser state (UI thread only) -------------------------------------

  // Serializes |window|'s tabs and writes them to disk as workspace |id|,
  // updating its tab count and modified time.  Does nothing if |id| is
  // unknown.
  void SaveWorkspaceFromWindow(const base::Uuid& id,
                               BrowserWindowInterface* window);

  // Shows workspace |id|, hiding |from_window|.  If |id| already has a window
  // this just swaps visibility; otherwise a window is created and the
  // workspace is read off disk into it.  Does nothing if |id| is unknown or
  // already shown in |from_window|.
  void SwitchWindowToWorkspace(const base::Uuid& id,
                               BrowserWindowInterface* from_window);

  // Shows the unmanaged window |from_window| was opened from, hiding
  // |from_window|.  Does nothing if |from_window| is itself unmanaged.
  void SwitchWindowToUnmanaged(BrowserWindowInterface* from_window);

  // Creates a workspace named |name| in a new, empty window and shows it,
  // hiding |from_window|.
  WorkspaceMetadata CreateWorkspaceInWindow(
      const std::string& name,
      BrowserWindowInterface* from_window);

  // Takes over shutdown when |window| is the last window on screen.  Returns
  // true when the caller should refuse this close request: the service then
  // closes every window itself, workspaces first.  SessionService keeps only
  // the window that closes last, so this ordering is what decides whether
  // "continue where you left off" restores the unmanaged tabs or a
  // workspace's.
  bool InterceptWindowClose(BrowserWindowInterface* window);

  base::WeakPtr<WorkspaceService> GetWeakPtr();

  // KeyedService:
  void Shutdown() override;

  // BrowserCollectionObserver:
  void OnBrowserClosed(BrowserWindowInterface* browser) override;

  // TabStripModelObserver:
  void WillCloseAllTabs(TabStripModel* tab_strip_model) override;

 private:
  // Records that |window| now shows |id| and starts watching its tab strip,
  // so the workspace can be saved before those tabs are torn down.
  void BindWorkspaceToWindow(const base::Uuid& id,
                             BrowserWindowInterface* window);

  // Returns the window of this profile whose tab strip is |tab_strip_model|.
  BrowserWindowInterface* FindWindowForTabStrip(
      TabStripModel* tab_strip_model) const;

  // Opens a window for this profile sized and placed over |from_window|,
  // carrying a single new tab.  A window with an empty tab strip is not a
  // thing the browser supports, so every workspace window starts with one.
  BrowserWindowInterface* CreateWindowOver(BrowserWindowInterface* from_window);

  // Opens a window for workspace |id| and fills it from disk.  |from_window|
  // supplies the bounds so the workspace appears where the person was looking.
  void OpenWorkspaceWindow(const base::Uuid& id,
                           BrowserWindowInterface* from_window);

  // Called on the UI thread with the commands read from disk by
  // OpenWorkspaceWindow.  |window_id| identifies the window just created,
  // which may have closed while the read was in flight.
  void DoFillWorkspaceWindow(
      base::Uuid id,
      SessionID window_id,
      std::vector<std::unique_ptr<sessions::SessionCommand>> commands);

  // Hides |from_window| and shows |to_window|, carrying the unmanaged-window
  // lineage across so a later switch back can find its way home.
  void SwapVisibleWindow(BrowserWindowInterface* from_window,
                         BrowserWindowInterface* to_window);

  // True when no normal window of this profile other than |ignored| is on
  // screen.  Pass nullptr to consider every window.
  bool HasNoVisibleWindowsExcept(BrowserWindowInterface* ignored) const;

  // Closes workspace windows and then unmanaged ones, skipping the close
  // confirmation for both since the person already asked to quit.
  void CloseAllWindowsInOrder();

  // Saves every hidden workspace window and closes it, so that hidden windows
  // never keep the browser running once the last visible one is gone.
  void CloseHiddenWindows();

  raw_ref<Profile> profile_;
  const base::FilePath workspaces_path_;
  raw_ref<PrefService> pref_service_;
  scoped_refptr<base::SequencedTaskRunner> io_task_runner_;

  // The window each opened workspace owns.  Small enough that the reverse
  // lookup is a linear scan rather than a second map to keep in sync.
  std::map<base::Uuid, SessionID> workspace_to_window_;

  // For each workspace window, the unmanaged window it was opened from.
  // Carried forward when switching workspace to workspace, so that row always
  // points at the window the person actually started in.
  std::map<SessionID, SessionID> window_to_unmanaged_window_;

  // Set once the service is driving shutdown, so the closes it issues are not
  // mistaken for fresh close requests.
  bool closing_all_windows_ = false;

  base::RepeatingClosureList workspaces_changed_callbacks_;

  base::ScopedObservation<ProfileBrowserCollection, BrowserCollectionObserver>
      browser_collection_observation_{this};

  base::WeakPtrFactory<WorkspaceService> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_WORKSPACES_WORKSPACE_SERVICE_H_
