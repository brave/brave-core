/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WORKSPACES_WORKSPACE_SESSION_UTILS_H_
#define BRAVE_BROWSER_WORKSPACES_WORKSPACE_SESSION_UTILS_H_

#include <memory>
#include <vector>

#include "components/sessions/core/session_command.h"
#include "components/sessions/core/session_id.h"

class BrowserWindowInterface;
struct WorkspaceMetadata;

// Serializes the tab strip of |window| into session commands and returns the
// result.  |workspace.number_of_tabs| is set to what was captured.  Returns an
// empty vector if the window has no tabs.
//
// Window geometry is deliberately not captured: a workspace owns tabs, and the
// window it is loaded into keeps its own bounds.
std::vector<std::unique_ptr<sessions::SessionCommand>>
SerializeWindowForWorkspace(BrowserWindowInterface* window,
                            WorkspaceMetadata& workspace);

// Deserializes |commands| into |window|, preserving pinned state, tab groups
// and navigation history.  |window| arrives holding the placeholder tab it was
// created with; the saved tabs are appended after it and the placeholder is
// then closed, in that order, because emptying a tab strip closes the window.
// Must be called on the UI thread.
//
// A placeholder whose page runs a beforeunload handler can outlive this call,
// since TabStripModel closes those asynchronously.
void RestoreWorkspaceIntoWindow(
    BrowserWindowInterface* window,
    std::vector<std::unique_ptr<sessions::SessionCommand>> commands);

#endif  // BRAVE_BROWSER_WORKSPACES_WORKSPACE_SESSION_UTILS_H_
