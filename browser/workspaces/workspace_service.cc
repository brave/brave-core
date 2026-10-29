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

bool WorkspaceService::IsWorkspaceOnScreenElsewhere(const base::Uuid& id,
                                                    SessionID window_id) const {
  std::optional<SessionID> host = GetWindowForWorkspace(id);
  if (!host || *host == window_id) {
    return false;
  }

  BrowserWindowInterface* host_window =
      BrowserWindowInterface::FromSessionID(*host);
  return host_window && host_window->GetWindow()->IsVisible();
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
  std::optional<SessionID> origin = GetUnmanagedWindowFor(from_id);
  window_to_unmanaged_window_.insert_or_assign(to_window->GetSessionID(),
                                               origin.value_or(from_id));

  // Take the outgoing window's bounds so the workspace appears where the
  // person was already looking rather than wherever it last sat.
  to_window->GetWindow()->SetBounds(from_window->GetWindow()->GetBounds());
  to_window->GetWindow()->Show();
  from_window->GetWindow()->Hide();
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

  BindWorkspaceToWindow(id, to_window);
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
    std::vector<std::unique_ptr<sessions::SessionCommand>> commands) {
  BrowserWindowInterface* window =
      BrowserWindowInterface::FromSessionID(window_id);
  if (!window) {
    workspace_to_window_.erase(id);
    return;
  }

  if (commands.empty()) {
    // Nothing saved yet, so the window keeps the new tab it opened with.
    return;
  }

  const int placeholder_tabs = window->GetTabStripModel()->count();
  RestoreWorkspaceIntoWindow(window, std::move(commands));
  // Retire the new tab the window opened with, now that real tabs are in.
  CloseLeadingTabs(window, placeholder_tabs);
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

  BrowserWindowInterface* to_window = CreateWindowOver(from_window);
  if (!to_window) {
    return workspace;
  }

  BindWorkspaceToWindow(workspace.id, to_window);
  SwapVisibleWindow(from_window, to_window);
  return workspace;
}

bool WorkspaceService::HasNoVisibleWindowsExcept(
    BrowserWindowInterface* ignored) const {
  auto* collection =
      ProfileBrowserCollection::GetForProfile(base::to_address(profile_));
  if (!collection) {
    return true;
  }

  bool any_visible = false;
  collection->ForEach([&](BrowserWindowInterface* browser) {
    if (browser != ignored &&
        browser->GetType() == BrowserWindowInterface::Type::TYPE_NORMAL &&
        browser->GetWindow()->IsVisible()) {
      any_visible = true;
      return false;
    }
    return true;
  });
  return !any_visible;
}

bool WorkspaceService::InterceptWindowClose(BrowserWindowInterface* window) {
  // The closes issued by CloseAllWindowsInOrder must run normally.
  if (closing_all_windows_) {
    return false;
  }

  // Closing this window only ends the session if nothing else is on screen.
  if (!HasNoVisibleWindowsExcept(window)) {
    return false;
  }

  // With no workspace windows open there is no ordering to arrange.
  if (workspace_to_window_.empty()) {
    return false;
  }

  closing_all_windows_ = true;
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&WorkspaceService::CloseAllWindowsInOrder, GetWeakPtr()));
  return true;
}

void WorkspaceService::CloseAllWindowsInOrder() {
  auto* collection =
      ProfileBrowserCollection::GetForProfile(base::to_address(profile_));
  if (!collection) {
    return;
  }

  std::vector<raw_ptr<BrowserWindowInterface>> workspace_windows;
  std::vector<raw_ptr<BrowserWindowInterface>> unmanaged_windows;
  collection->ForEach([&](BrowserWindowInterface* browser) {
    if (browser->GetType() != BrowserWindowInterface::Type::TYPE_NORMAL) {
      return true;
    }
    if (GetWorkspaceForWindow(browser->GetSessionID())) {
      workspace_windows.push_back(browser);
    } else {
      unmanaged_windows.push_back(browser);
    }
    return true;
  });

  // Workspaces close first, while an unmanaged window is still open, so
  // SessionService records a close command for each and forgets them. The
  // unmanaged windows then close last and are the ones it keeps. Each window
  // saves itself on the way out through WillCloseAllTabs.
  for (BrowserWindowInterface* browser : workspace_windows) {
    UnloadController::From(browser)->set_confirmed_to_close(true);
    browser->GetWindow()->Close();
  }
  for (BrowserWindowInterface* browser : unmanaged_windows) {
    UnloadController::From(browser)->set_confirmed_to_close(true);
    browser->GetWindow()->Close();
  }
}

void WorkspaceService::CloseHiddenWindows() {
  auto* collection =
      ProfileBrowserCollection::GetForProfile(base::to_address(profile_));
  if (!collection) {
    return;
  }

  std::vector<raw_ptr<BrowserWindowInterface>> hidden;
  collection->ForEach([&](BrowserWindowInterface* browser) {
    if (!browser->GetWindow()->IsVisible()) {
      hidden.push_back(browser);
    }
    return true;
  });

  // Closing runs each window through WillCloseAllTabs, which is what saves it.
  for (BrowserWindowInterface* browser : hidden) {
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
}

BrowserWindowInterface* WorkspaceService::FindWindowForTabStrip(
    TabStripModel* tab_strip_model) const {
  auto* collection =
      ProfileBrowserCollection::GetForProfile(base::to_address(profile_));
  if (!collection) {
    return nullptr;
  }

  BrowserWindowInterface* found = nullptr;
  collection->ForEach([&](BrowserWindowInterface* browser) {
    if (browser->GetTabStripModel() == tab_strip_model) {
      found = browser;
      return false;
    }
    return true;
  });
  return found;
}

void WorkspaceService::WillCloseAllTabs(TabStripModel* tab_strip_model) {
  // The last chance to capture a workspace: these tabs are about to go.
  BrowserWindowInterface* window = FindWindowForTabStrip(tab_strip_model);
  if (!window) {
    return;
  }
  if (std::optional<base::Uuid> id =
          GetWorkspaceForWindow(window->GetSessionID())) {
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
  window_to_unmanaged_window_.erase(window_id);
  std::erase_if(window_to_unmanaged_window_, [window_id](const auto& entry) {
    return entry.second == window_id;
  });

  // Hidden windows must not keep the browser running. Once the last visible
  // window is gone, retire them so normal shutdown can proceed. Deferred
  // because this runs while |browser| is still being torn down.
  if (HasNoVisibleWindowsExcept(nullptr)) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(&WorkspaceService::CloseHiddenWindows, GetWeakPtr()));
  }
}
