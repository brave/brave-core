/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WORKSPACE_BRAVE_WORKSPACE_H_
#define BRAVE_BROWSER_WORKSPACE_BRAVE_WORKSPACE_H_

#include <string>

#include "base/time/time.h"

// Lightweight summary returned by ListWorkspaces(), used to populate UI.
// The full session state is stored as Chromium session commands on disk.
struct WorkspaceInfo {
  WorkspaceInfo();
  ~WorkspaceInfo();

  std::string name;
  base::Time created_at;
};

#endif  // BRAVE_BROWSER_WORKSPACE_BRAVE_WORKSPACE_H_
