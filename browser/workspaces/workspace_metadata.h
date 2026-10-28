/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WORKSPACES_WORKSPACE_METADATA_H_
#define BRAVE_BROWSER_WORKSPACES_WORKSPACE_METADATA_H_

#include <string>

#include "base/time/time.h"
#include "base/uuid.h"

// Lightweight summary returned by ListWorkspaces(), used to populate UI.
// The full session state is stored as Chromium session commands on disk.
//
// |id| is the stable identity: it keys both the metadata preference and the
// on-disk directory, so renaming a workspace moves no storage. A workspace
// holds the tabs of a single window, so there is no window count here.
struct WorkspaceMetadata {
  base::Uuid id;
  std::string name;
  base::Time modified_at;
  int number_of_tabs = 0;
};

#endif  // BRAVE_BROWSER_WORKSPACES_WORKSPACE_METADATA_H_
