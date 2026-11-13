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
#include "base/memory/raw_ptr.h"
#include "base/task/bind_post_task.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "base/uuid.h"
#include "brave/browser/workspaces/workspace_session_utils.h"
#include "brave/browser/workspaces/workspace_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/create_browser_window.h"
#include "chrome/browser/ui/browser_window/public/profile_browser_collection.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/browser/ui/unload_controller.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/tabs/public/tab_interface.h"
#include "ui/base/base_window.h"
#include "url/gurl.h"

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
  // The pref is keyed by the workspace id, so this is a dictionary lookup
  // rather than a scan of the whole list.
  const std::string key = id.AsLowercaseString();
  const base::Value* entry =
      pref_service_->GetDict(kWorkspacesMetadataPref).Find(key);
  return entry ? WorkspaceMetadataFromDictEntry(key, *entry) : std::nullopt;
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
  workspaces_changed_callbacks_.Notify();
}

void WorkspaceService::RollBackWorkspaceMetadata(
    const WorkspaceMetadata& previous,
    base::Time written_at) {
  // The write failed, but it failed asynchronously: the workspace may have
  // been renamed, saved again or deleted in the meantime. Only undo the entry
  // this write put there, so a rollback never overwrites a newer change or
  // puts a deleted workspace back in the list.
  std::optional<WorkspaceMetadata> current = GetWorkspace(previous.id);
  if (!current || current->modified_at != written_at) {
    return;
  }
  SaveWorkspaceMetadata(previous);
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
  std::optional<SessionID> window_id = GetWindowForWorkspace(id);
  BrowserWindowInterface* window =
      window_id ? BrowserWindowInterface::FromSessionID(*window_id) : nullptr;
  if (!window) {
    FinishDeletingWorkspace(id);
    return;
  }

  // A deleted workspace always takes its window with it. Left behind, that
  // window is saved by nobody: it is excluded from Chromium's session restore,
  // and it is no longer ours to save. A put-away one would also linger off
  // screen with no row in any bubble to bring it back.
  //
  // Nothing is torn down until the window has actually gone. Close() is
  // asynchronous, so removing the metadata and directory here would do it
  // while the window is still holding those tabs; the subscription covers the
  // close being refused, which the flags below make unlikely but not
  // impossible.
  workspaces_pending_delete_.insert_or_assign(
      id, window->RegisterBrowserCloseCancelled(base::BindRepeating(
              &WorkspaceService::OnWorkspaceWindowCloseCancelled, GetWeakPtr(),
              id)));

  if (!put_away_windows_.contains(*window_id)) {
    // Someone is looking at this window, so they need somewhere to land
    // before it goes.
    ShowUnmanagedWindowFor(window);
  }
  put_away_windows_.erase(*window_id);

  // Confirming the deletion already answered anything closing would ask, so
  // the window goes without consulting its pages. The two flags cover
  // different prompts: set_confirmed_to_close() skips Brave's own "close all
  // tabs?" confirmation, and set_force_skip_warning_user_on_close() skips a
  // page's beforeunload handler along with the in-progress-download warning.
  //
  // Both paths are treated alike deliberately. A put-away window would
  // otherwise prompt about a page nobody can see, and a visible one would
  // offer to cancel a deletion the person has already confirmed.
  auto* unload_controller = UnloadController::From(window);
  unload_controller->set_confirmed_to_close(true);
  unload_controller->set_force_skip_warning_user_on_close(true);
  window->GetWindow()->Close();
}

void WorkspaceService::OnWorkspaceWindowCloseCancelled(
    base::Uuid id,
    BrowserWindowInterface* window,
    BrowserWindowInterface::ClosingStatus status) {
  // Unload handlers still running is not a refusal; the close may yet go
  // through.
  if (status ==
      BrowserWindowInterface::ClosingStatus::kDeniedUnloadHandlersNeedTime) {
    return;
  }

  // Something refused the close even though beforeunload and the download
  // warning were skipped. The window is staying, so the workspace stays with
  // it: still listed, still bound, still saved. The person can delete again.
  workspaces_pending_delete_.erase(id);
}

void WorkspaceService::FinishDeletingWorkspace(const base::Uuid& id) {
  RemoveWorkspaceMetadata(id);
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
  const size_t removed = std::erase_if(
      workspace_to_window_,
      [window_id](const auto& entry) { return entry.second == window_id; });
  if (removed > 0) {
    workspaces_changed_callbacks_.Notify();
  }
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
      base::BindOnce(&WorkspaceService::RollBackWorkspaceMetadata, GetWeakPtr(),
                     *previous, workspace.modified_at));

  io_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&WriteWorkspaceToDisk, std::move(commands),
                                std::move(workspace_path), std::move(backend),
                                std::move(on_error)));
}

std::string WorkspaceService::GetWorkspaceNameForWindow(
    SessionID window_id) const {
  std::optional<base::Uuid> id = GetWorkspaceForWindow(window_id);
  if (!id) {
    return std::string();
  }
  std::optional<WorkspaceMetadata> workspace = GetWorkspace(*id);
  return workspace ? workspace->name : std::string();
}

base::CallbackListSubscription WorkspaceService::RegisterWorkspacesChanged(
    base::RepeatingClosure callback) {
  return workspaces_changed_callbacks_.Add(std::move(callback));
}

bool WorkspaceService::IsWorkspaceOnScreenElsewhere(const base::Uuid& id,
                                                    SessionID window_id) const {
  std::optional<SessionID> host = GetWindowForWorkspace(id);
  if (!host || *host == window_id) {
    return false;
  }

  // A window the person minimized still counts as theirs, so only one this
  // service put away releases its workspace.
  return BrowserWindowInterface::FromSessionID(*host) &&
         !put_away_windows_.contains(*host);
}

std::optional<SessionID> WorkspaceService::GetUnmanagedWindowFor(
    SessionID window_id) const {
  auto it = window_to_unmanaged_window_.find(window_id);
  if (it == window_to_unmanaged_window_.end()) {
    return std::nullopt;
  }
  return it->second;
}

void WorkspaceService::SwapVisibleWindow(BrowserWindowInterface* from_window,
                                         BrowserWindowInterface* to_window) {
  if (from_window == to_window) {
    return;
  }

  // A workspace window inherits its origin, so switching from one workspace to
  // another still knows which unmanaged window to go home to.
  const SessionID from_id = from_window->GetSessionID();
  const SessionID to_id = to_window->GetSessionID();
  const SessionID origin = GetUnmanagedWindowFor(from_id).value_or(from_id);
  if (to_id != origin) {
    window_to_unmanaged_window_.insert_or_assign(to_id, origin);
  }

  // Capture the window being put away. Until autosave lands this is the only
  // moment besides closing where a workspace is written, and a window can sit
  // off screen indefinitely -- losing everything since it was opened if the
  // browser then crashes, since these windows are excluded from Chromium's
  // session restore.
  if (std::optional<base::Uuid> leaving = GetWorkspaceForWindow(from_id)) {
    SaveWorkspaceFromWindow(*leaving, from_window);
  }

  // Take the outgoing window's bounds so the workspace appears where the
  // person was already looking rather than wherever it last sat.
  to_window->GetWindow()->SetBounds(from_window->GetWindow()->GetBounds());
  to_window->GetWindow()->Show();
  put_away_windows_.erase(to_id);
  from_window->GetWindow()->Hide();
  put_away_windows_.insert(from_id);
}

void WorkspaceService::SwitchWindowToWorkspace(
    const base::Uuid& id,
    BrowserWindowInterface* from_window) {
  if (!GetWorkspace(id)) {
    return;
  }

  const SessionID from_id = from_window->GetSessionID();
  if (GetWorkspaceForWindow(from_id) == id) {
    return;
  }

  std::optional<SessionID> existing = GetWindowForWorkspace(id);
  if (!existing) {
    OpenWorkspaceWindow(id, from_window);
    return;
  }

  BrowserWindowInterface* to_window =
      BrowserWindowInterface::FromSessionID(*existing);
  if (!to_window) {
    // The window went away without us hearing about it.
    workspace_to_window_.erase(id);
    OpenWorkspaceWindow(id, from_window);
    return;
  }

  SwapVisibleWindow(from_window, to_window);
}

BrowserWindowInterface* WorkspaceService::CreateWindowOver(
    BrowserWindowInterface* from_window,
    WindowKind kind) {
  if (GetBrowserWindowCreationStatusForProfile(*profile_) !=
      BrowserWindowInterface::CreationStatus::kOk) {
    return nullptr;
  }

  BrowserWindowCreateParams params(BrowserWindowInterface::TYPE_NORMAL,
                                   base::to_address(profile_),
                                   /*user_gesture=*/true);
  params.initial_bounds = from_window->GetWindow()->GetBounds();
  params.should_trigger_session_restore = false;
  // Keep workspace windows out of Chromium's session entirely: this service
  // persists them itself, and leaving them tracked means "continue where you
  // left off" restores a workspace as though it were ordinary tabs. An
  // unmanaged window is the opposite case -- nothing here saves it, so it has
  // to stay tracked.
  params.omit_from_session_restore = kind == WindowKind::kWorkspace;
  BrowserWindowInterface* window = CreateBrowserWindow(std::move(params));
  if (!window) {
    return nullptr;
  }

  // CreateBrowserWindow leaves the tab strip empty. A new space has nothing to
  // restore, and a restored one closes this tab once its own are in place.
  chrome::AddTabAt(window, GURL(), /*index=*/-1, /*foreground=*/true);
  return window;
}

void WorkspaceService::ShowUnmanagedWindowFor(BrowserWindowInterface* window) {
  if (std::optional<SessionID> home_id =
          GetUnmanagedWindowFor(window->GetSessionID())) {
    if (auto* home = BrowserWindowInterface::FromSessionID(*home_id)) {
      put_away_windows_.erase(*home_id);
      home->GetWindow()->SetBounds(window->GetWindow()->GetBounds());
      home->GetWindow()->Show();
      return;
    }
  }

  // The window this one came from has since closed, so there is nothing to go
  // back to. Open an ordinary window rather than leaving the person with
  // nothing once |window| goes.
  if (auto* home = CreateWindowOver(window, WindowKind::kUnmanaged)) {
    home->GetWindow()->Show();
  }
}

void WorkspaceService::OpenWorkspaceWindow(
    const base::Uuid& id,
    BrowserWindowInterface* from_window) {
  // The tabs are read before the window exists, so the window can be created
  // already holding them. Creating it first and filling it afterwards would
  // put an empty, bound, interactive window on screen for the length of a
  // disk read: long enough for its tabs to be closed before they are
  // saveable, for the workspace to be deleted underneath it, or for tabs the
  // person opens to be mistaken for placeholders and closed.
  base::FilePath path = GetWorkspacePathForId(id);
  auto backend = base::MakeRefCounted<sessions::CommandStorageBackend>(
      io_task_runner_, path, kWorkspaceSessionType,
      /*encryptor=*/nullptr);

  io_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&ReadWorkspaceFromDisk, std::move(path),
                     std::move(backend)),
      base::BindOnce(&WorkspaceService::DoOpenWorkspaceWindow, GetWeakPtr(), id,
                     from_window->GetSessionID()));
}

void WorkspaceService::DoOpenWorkspaceWindow(
    base::Uuid id,
    SessionID from_window_id,
    std::optional<std::vector<std::unique_ptr<sessions::SessionCommand>>>
        commands) {
  if (!commands) {
    // The workspace could not be read. No window is created, so there is
    // nothing holding tabs that neither this service nor Chromium would save.
    return;
  }

  // Any of this can have changed while the read was in flight.
  if (!GetWorkspace(id) || GetWindowForWorkspace(id)) {
    return;
  }

  BrowserWindowInterface* from_window =
      BrowserWindowInterface::FromSessionID(from_window_id);
  if (!from_window) {
    return;
  }

  BrowserWindowInterface* to_window =
      CreateWindowOver(from_window, WindowKind::kWorkspace);
  if (!to_window) {
    return;
  }

  // Bound before the swap, so the window is saveable from the moment the
  // person can reach it.
  BindWorkspaceToWindow(id, to_window);
  if (!commands->empty()) {
    RestoreWorkspaceIntoWindow(to_window, std::move(*commands));
  }
  SwapVisibleWindow(from_window, to_window);
}

void WorkspaceService::SwitchWindowToUnmanaged(
    BrowserWindowInterface* from_window) {
  std::optional<SessionID> unmanaged_window_id =
      GetUnmanagedWindowFor(from_window->GetSessionID());
  if (!unmanaged_window_id) {
    return;
  }

  BrowserWindowInterface* to_window =
      BrowserWindowInterface::FromSessionID(*unmanaged_window_id);
  if (!to_window) {
    window_to_unmanaged_window_.erase(from_window->GetSessionID());
    return;
  }

  SwapVisibleWindow(from_window, to_window);
}

WorkspaceMetadata WorkspaceService::CreateWorkspaceInWindow(
    const std::string& name,
    BrowserWindowInterface* from_window) {
  WorkspaceMetadata workspace = CreateWorkspace(name);

  // Deliberately not routed through SwitchWindowToWorkspace: a new workspace
  // has nothing on disk, so it needs no read, and waiting on one would leave
  // the window unsaveable for the length of a round trip it does not need.
  BrowserWindowInterface* to_window =
      CreateWindowOver(from_window, WindowKind::kWorkspace);
  if (!to_window) {
    return workspace;
  }

  BindWorkspaceToWindow(workspace.id, to_window);
  SwapVisibleWindow(from_window, to_window);
  return workspace;
}

bool WorkspaceService::HasOnlyPutAwayWindowsExcept(
    BrowserWindowInterface* ignored) const {
  auto* collection =
      ProfileBrowserCollection::GetForProfile(base::to_address(profile_));
  if (!collection) {
    return true;
  }

  bool any_reachable = false;
  collection->ForEach([&](BrowserWindowInterface* browser) {
    if (browser != ignored &&
        browser->GetType() == BrowserWindowInterface::Type::TYPE_NORMAL &&
        !put_away_windows_.contains(browser->GetSessionID())) {
      any_reachable = true;
      return false;
    }
    return true;
  });
  return !any_reachable;
}

void WorkspaceService::ClosePutAwayWindows() {
  // Every one of these is going away, so none of them has an origin left to
  // hand the screen back to. Dropping the lineage up front stops
  // OnBrowserClosed from reading each of these closes as a workspace window
  // going away on its own and showing the next window instead of quitting.
  // It has to happen here rather than around the loop, because the closes
  // below complete asynchronously.
  window_to_unmanaged_window_.clear();

  std::vector<raw_ptr<BrowserWindowInterface>> put_away;
  for (SessionID window_id : put_away_windows_) {
    if (auto* browser = BrowserWindowInterface::FromSessionID(window_id)) {
      put_away.push_back(browser);
    }
  }

  // Closing runs each window through WillCloseAllTabs, which is what saves it.
  for (BrowserWindowInterface* browser : put_away) {
    // Mark the close as already confirmed. These windows are off screen, so a
    // prompt naming their tabs would come out of nowhere; the person answered
    // for the window they were actually looking at. This is the same signal
    // WindowClosingConfirmController sets once its dialog is accepted.
    UnloadController::From(browser)->set_confirmed_to_close(true);
    browser->GetWindow()->Close();
  }
}

void WorkspaceService::BindWorkspaceToWindow(const base::Uuid& id,
                                             BrowserWindowInterface* window) {
  workspace_to_window_.insert_or_assign(id, window->GetSessionID());
  window->GetTabStripModel()->AddObserver(this);
  workspaces_changed_callbacks_.Notify();
}

void WorkspaceService::WillCloseAllTabs(TabStripModel* tab_strip_model) {
  // The last chance to capture a workspace: these tabs are about to go, and
  // they are still here, so one of them can name the window they live in.
  tabs::TabInterface* tab = tab_strip_model->GetTabAtIndex(0);
  BrowserWindowInterface* window =
      tab ? tab->GetBrowserWindowInterface() : nullptr;
  if (!window) {
    return;
  }

  const SessionID window_id = window->GetSessionID();
  std::optional<base::Uuid> id = GetWorkspaceForWindow(window_id);
  // A workspace on its way out is not worth writing back first.
  if (id && !workspaces_pending_delete_.contains(*id)) {
    SaveWorkspaceFromWindow(*id, window);
  }
}

void WorkspaceService::NoteWindowCloseRequested(
    BrowserWindowInterface* window) {
  // A window whose tab strip has already emptied is closing because it ran
  // out of tabs: Browser::TabStripEmpty() closes it through the same path a
  // person's close request takes. Tabs still being present is what marks this
  // as the person's doing.
  if (window->GetTabStripModel()->count() > 0) {
    windows_closed_by_person_.insert(window->GetSessionID());
  }
}

base::WeakPtr<WorkspaceService> WorkspaceService::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void WorkspaceService::Shutdown() {
  browser_collection_observation_.Reset();
  // Tab strips are observed through TabStripModelObserver's own bookkeeping
  // rather than a ScopedObservation member, which TabStripModel deletes the
  // constructor for. Without this, WillCloseAllTabs could still run
  // SaveWorkspaceFromWindow -- writing prefs and posting disk I/O -- after
  // the service has shut down.
  TabStripModelObserver::StopObservingAll(this);
  weak_ptr_factory_.InvalidateWeakPtrs();
}

void WorkspaceService::OnBrowserClosed(BrowserWindowInterface* browser) {
  // By now every tab is already destroyed, so there is nothing left to
  // serialize here; WillCloseAllTabs saved the workspace while it still had
  // tabs.
  const SessionID window_id = browser->GetSessionID();

  // Both read before the bindings are dropped below.
  const std::optional<SessionID> origin = GetUnmanagedWindowFor(window_id);
  const std::optional<base::Uuid> held = GetWorkspaceForWindow(window_id);

  UnbindWindow(window_id);
  put_away_windows_.erase(window_id);
  std::erase_if(window_to_unmanaged_window_, [window_id](const auto& entry) {
    return entry.first == window_id || entry.second == window_id;
  });

  // The window holding a workspace queued for deletion has gone, so there is
  // nothing left that the removal could strand.
  if (held && workspaces_pending_delete_.erase(*held) > 0) {
    FinishDeletingWorkspace(*held);
  }

  // A workspace window going away on its own -- the person closed its last
  // tab -- is not a request to quit. Hand the screen back to the window it
  // was opened from instead of retiring everything behind it. Closing the
  // window outright is a request to quit, so that case falls through.
  const bool closed_by_person = windows_closed_by_person_.erase(window_id) > 0;
  if (origin && !closed_by_person) {
    if (auto* home = BrowserWindowInterface::FromSessionID(*origin)) {
      put_away_windows_.erase(*origin);
      home->GetWindow()->Show();
      return;
    }
  }

  // Put-away windows must not keep the browser running. Once nothing the
  // person can reach is left, retire them so normal shutdown can proceed.
  // Deferred because this runs while |browser| is still being torn down.
  if (HasOnlyPutAwayWindowsExcept(nullptr)) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(&WorkspaceService::ClosePutAwayWindows, GetWeakPtr()));
  }
}
