/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_SYNC_SCHEDULER_IMPL_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_SYNC_SCHEDULER_IMPL_H_

namespace syncer {

extern const char kNigoriFolderNotReadyError[];

}  // namespace syncer

#include "components/sync/engine/sync_protocol_error.h"

#define DoPollSyncCycleJob                                                   \
  HandleBraveConfigurationFailure(                                           \
      const ModelNeutralState& model_neutral_state);                         \
  void SchedulePermanentlyDeleteAccount(                                     \
      base::OnceCallback<void(const SyncProtocolError&)> callback) override; \
  void PermanentlyDeleteAccountImpl(                                         \
      base::OnceCallback<void(const SyncProtocolError&)> callback);          \
  void DoPollSyncCycleJob

#include "src/components/sync/engine/sync_scheduler_impl.h"  // IWYU pragma: export

#undef DoPollSyncCycleJob

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_SYNC_SCHEDULER_IMPL_H_
