/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WORKSPACE_WORKSPACE_SESSION_UTILS_H_
#define BRAVE_BROWSER_WORKSPACE_WORKSPACE_SESSION_UTILS_H_

#include <memory>
#include <vector>

#include "components/sessions/core/session_command.h"
#include "components/sessions/core/session_id.h"
#include "ui/base/mojom/window_show_state.mojom.h"
#include "ui/gfx/geometry/rect.h"

class TabStripModel;

// Appends session commands for a single browser window to |commands|.
// Serializes window type, bounds, tab groups, tabs (with full navigation
// history), pinned state, and the active tab index.
void AppendBrowserSessionCommands(
    const SessionID& window_id,
    TabStripModel* tsm,
    gfx::Rect restored_bounds,
    ui::mojom::WindowShowState restored_state,
    std::vector<std::unique_ptr<sessions::SessionCommand>>& commands);

#endif  // BRAVE_BROWSER_WORKSPACE_WORKSPACE_SESSION_UTILS_H_
