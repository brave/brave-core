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
#include "ui/base/mojom/window_show_state.mojom.h"
#include "ui/gfx/geometry/rect.h"

class Profile;
class TabStripModel;
struct WorkspaceMetadata;

// Iterates all normal browser windows for |profile|, serializes their state
// into session commands, and returns the result.  |window_count| and
// |tab_count| are incremented to reflect what was captured.  Returns an empty
// vector if there are no tabs to save.
std::vector<std::unique_ptr<sessions::SessionCommand>>
GenerateBrowserSessionCommandsForWorkspace(Profile* profile,
                                           WorkspaceMetadata& workspace);

// Deserializes |commands| and opens the encoded windows/tabs in new browser
// windows belonging to |profile|.  Must be called on the UI thread.
void RestoreBrowserSessionCommandsForWorkspace(
    Profile* profile,
    std::vector<std::unique_ptr<sessions::SessionCommand>> commands);

#endif  // BRAVE_BROWSER_WORKSPACES_WORKSPACE_SESSION_UTILS_H_
