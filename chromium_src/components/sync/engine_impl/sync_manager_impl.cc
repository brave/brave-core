/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/sync/engine_impl/sync_scheduler_impl.h"

#define BRAVE_SYNC_MANAGER_IMPL_INIT                \
  static_cast<SyncSchedulerImpl*>(scheduler_.get()) \
      ->nudge_sync_cycle_delegate_function_ =       \
      args->nudge_sync_cycle_delegate_function;     \
  static_cast<SyncSchedulerImpl*>(scheduler_.get()) \
      ->poll_sync_cycle_delegate_function_ =        \
      args->poll_sync_cycle_delegate_function;

#include "../../../../../components/sync/engine_impl/sync_manager_impl.cc"  // NOLINT
#undef BRAVE_SYNC_MANAGER_IMPL_INIT
