/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_IMPL_CYCLE_SYNC_CYCLE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_IMPL_CYCLE_SYNC_CYCLE_H_

#include "brave/components/brave_sync/jslib_messages_fwd.h"

#define BRAVE_SYNC_CYCLE_DELEGATE_H \
virtual void OnNudgeSyncCycle(brave_sync::RecordsListPtr records_list) {} \
virtual void OnPollSyncCycle(brave_sync::GetRecordsCallback cb, \
                             base::WaitableEvent* wevent) {}

#include "../../../../../../components/sync/engine_impl/cycle/sync_cycle.h"
#undef BRAVE_SYNC_CYCLE_DELEGATE_H

#endif    // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_IMPL_CYCLE_SYNC_CYCLE_H_
