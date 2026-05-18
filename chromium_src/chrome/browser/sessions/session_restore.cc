/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/sessions/brave_tree_tab_restore_helper.h"

// Called inside SessionRestoreImpl::ProcessSessionWindows after
// RestoreTabGroupMetadata, where |browser|, |window|, and |initial_tab_count|
// are all in scope.
#define BRAVE_RESTORE_TREE_TAB_NODES \
  BraveRestoreTreeTabNodeMetadata(browser, window->tabs, initial_tab_count);

#include <chrome/browser/sessions/session_restore.cc>

#undef BRAVE_RESTORE_TREE_TAB_NODES
