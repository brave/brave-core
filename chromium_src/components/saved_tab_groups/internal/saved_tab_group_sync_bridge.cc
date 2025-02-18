/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define BRAVE_APPLY_DISABLE_SYNC_CHANGES_CLEAR_GROUPS \
  groups_to_close_locally.clear();                    \
  DCHECK(groups_to_close_locally.empty());
#include "src/components/saved_tab_groups/internal/saved_tab_group_sync_bridge.cc"
#undef BRAVE_APPLY_DISABLE_SYNC_CHANGES_CLEAR_GROUPS
