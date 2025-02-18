/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/saved_tab_groups/proto/local_tab_group_data.pb.h"
#include "components/saved_tab_groups/public/saved_tab_group.h"

namespace {
static bool get_true() {
  return true;
}
}  // namespace

#define created_before_syncing_tab_groups \
  created_before_syncing_tab_groups() || get_true

#include "src/components/saved_tab_groups/internal/saved_tab_group_sync_bridge.cc"

#undef created_before_syncing_tab_groups
