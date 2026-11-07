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
  // A deleted workspace takes its window with it. Left behind, a put-away
  // window would linger off screen with no row in any bubble to bring it
  // back, holding the browser open.
  if (std::optional<SessionID> window_id = GetWindowForWorkspace(id)) {
    BrowserWindowInterface* window =
        BrowserWindowInterface::FromSessionID(*window_id);
    // Unbinding first is what stops the close from saving the workspace back
    // out through WillCloseAllTabs.
    UnbindWindow(*window_id);

    const bool put_away = put_away_windows_.contains(*window_id);
    const bool can_go_home = GetUnmanagedWindowFor(*window_id).has_value();
    if (window && (put_away || can_go_home)) {
      if (!put_away) {
        // Someone is looking at this window, so send them home before it goes.
        SwitchWindowToUnmanaged(window);
      }
      put_away_windows_.erase(*window_id);
      UnloadController::From(window)->set_confirmed_to_close(true);
      window->GetWindow()->Close();
    }
    // A window that is on screen with nowhere to go stays open as an ordinary
    // unmanaged window: closing it would take away the only thing the person
    // can see.
  }

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
      base::BindOnce(&WorkspaceService::SaveWorkspaceMetadata, GetWeakPtr(),
                     *previous));

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
    BrowserWindowInterface* from_window) {
  if (GetBrowserWindowCreationStatusForProfile(*profile_) !=
      BrowserWindowInterface::CreationStatus::kOk) {
    return nullptr;
  }

  BrowserWindowCreateParams params(BrowserWindowInterface::TYPE_NORMAL,
                                   base::to_address(profile_),
                                   /*user_gesture=*/true);
  params.initial_bounds = from_window->GetWindow()->GetBounds();
  params.should_trigger_session_restore = false;
  // Keep workspace windows out of Chromium's session entirely. The service
  // persists them itself, and leaving them tracked means "continue where you
  // left off" restores a workspace as though it were ordinary tabs.
  params.omit_from_session_restore = true;
  BrowserWindowInterface* window = CreateBrowserWindow(std::move(params));
  if (!window) {
    return nullptr;
  }

  // CreateBrowserWindow leaves the tab strip empty. A new space has nothing to
  // restore, and a restored one closes this tab once its own are in place.
  chrome::AddTabAt(window, GURL(), /*index=*/-1, /*foreground=*/true);
  return window;
}

void WorkspaceService::OpenWorkspaceWindow(
    const base::Uuid& id,
    BrowserWindowInterface* from_window) {
  BrowserWindowInterface* to_window = CreateWindowOver(from_window);
  if (!to_window) {
    return;
  }

  BindWorkspaceToWindowForRestore(id, to_window);
  SwapVisibleWindow(from_window, to_window);

  // A workspace opened for the first time this session still lives on disk.
  base::FilePath path = GetWorkspacePathForId(id);
  auto backend = base::MakeRefCounted<sessions::CommandStorageBackend>(
      io_task_runner_, path, kWorkspaceSessionType,
      /*encryptor=*/nullptr);

  io_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&ReadWorkspaceFromDisk, std::move(path),
                     std::move(backend)),
      base::BindOnce(&WorkspaceService::DoFillWorkspaceWindow, GetWeakPtr(), id,
                     to_window->GetSessionID()));
}

void WorkspaceService::DoFillWorkspaceWindow(
    base::Uuid id,
    SessionID window_id,
    std::optional<std::vector<std::unique_ptr<sessions::SessionCommand>>>
        commands) {
  windows_awaiting_restore_.erase(window_id);

  BrowserWindowInterface* window =
      BrowserWindowInterface::FromSessionID(window_id);
  if (!window) {
    // Only release the binding if this workspace is still pointed at the
    // window we were filling; it may have been reopened somewhere else while
    // the read was in flight.
    auto it = workspace_to_window_.find(id);
    if (it != workspace_to_window_.end() && it->second == window_id) {
      workspace_to_window_.erase(it);
      workspaces_changed_callbacks_.Notify();
    }
    return;
  }

  if (!commands) {
    // The workspace could not be read. Leave it bound to nothing so the empty
    // window cannot save itself over whatever is still on disk.
    UnbindWindow(window_id);
    return;
  }

  if (commands->empty()) {
    // Nothing saved yet, so the window keeps the new tab it opened with.
    return;
  }

  RestoreWorkspaceIntoWindow(window, std::move(*commands));
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
  // A new workspace has nothing on disk, so opening it leaves the window with
  // just the tab it was created with.
  SwitchWindowToWorkspace(workspace.id, from_window);
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

void WorkspaceService::BindWorkspaceToWindowForRestore(
    const base::Uuid& id,
    BrowserWindowInterface* window) {
  BindWorkspaceToWindow(id, window);
  windows_awaiting_restore_.insert(window->GetSessionID());
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
  if (windows_awaiting_restore_.contains(window_id)) {
    // The window is still holding its placeholder tab while the workspace is
    // read off disk. Saving now would write that placeholder over it.
    return;
  }

  if (std::optional<base::Uuid> id = GetWorkspaceForWindow(window_id)) {
    SaveWorkspaceFromWindow(*id, window);
  }
}

base::WeakPtr<WorkspaceService> WorkspaceService::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void WorkspaceService::Shutdown() {
  browser_collection_observation_.Reset();
  weak_ptr_factory_.InvalidateWeakPtrs();
}

void WorkspaceService::OnBrowserClosed(BrowserWindowInterface* browser) {
  // By now every tab is already destroyed, so there is nothing left to
  // serialize here; WillCloseAllTabs saved the workspace while it still had
  // tabs.
  const SessionID window_id = browser->GetSessionID();
  UnbindWindow(window_id);
  put_away_windows_.erase(window_id);
  windows_awaiting_restore_.erase(window_id);
  std::erase_if(window_to_unmanaged_window_, [window_id](const auto& entry) {
    return entry.first == window_id || entry.second == window_id;
  });

  // Hidden windows must not keep the browser running. Once the last visible
  // window is gone, retire them so normal shutdown can proceed. Deferred
  // because this runs while |browser| is still being torn down.
  if (HasOnlyPutAwayWindowsExcept(nullptr)) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(&WorkspaceService::ClosePutAwayWindows, GetWeakPtr()));
  }
}
