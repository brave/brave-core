/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "brave/components/brave_sync/buildflags/buildflags.h"
#include "components/sync/driver/glue/sync_engine_impl.h"
#include "components/sync/engine/sync_engine_host.h"

#if BUILDFLAG(ENABLE_BRAVE_SYNC)
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_sync/jslib_messages.h"
#include "brave/components/brave_sync/public/brave_profile_sync_service.h"

using brave_sync::BraveProfileSyncService;
using brave_sync::GetRecordsCallback;
using brave_sync::RecordsList;
#endif

namespace syncer {

#if BUILDFLAG(ENABLE_BRAVE_SYNC)
SyncEngineHost* BraveGetSyncEngineHost(SyncEngineImpl* sync_engine) {
  return sync_engine->host_;
}

void OnNudgeSyncCycleOnOwnerThread(base::WeakPtr<SyncEngineImpl> sync_engine,
                                  brave_sync::RecordsListPtr records_list) {
  if (sync_engine.get())
    static_cast<BraveProfileSyncService*>(
        BraveGetSyncEngineHost(sync_engine.get()))->OnNudgeSyncCycle(
            std::move(records_list));
}

void OnNudgeSyncCycle(WeakHandle<SyncEngineImpl> sync_engine_impl,
                      brave_sync::RecordsListPtr records_list) {
  sync_engine_impl.Call(FROM_HERE,
                        &OnNudgeSyncCycleOnOwnerThread,
                        base::Passed(&records_list));
}

void OnPollSyncCycleOnOwnerThread(base::WeakPtr<SyncEngineImpl> sync_engine,
                                 GetRecordsCallback cb,
                                 base::WaitableEvent* wevent) {
  if (sync_engine.get())
    static_cast<BraveProfileSyncService*>(
        BraveGetSyncEngineHost(sync_engine.get()))->OnPollSyncCycle(cb, wevent);
}

void OnPollSyncCycle(WeakHandle<SyncEngineImpl> sync_engine_impl,
                     GetRecordsCallback cb,
                     base::WaitableEvent* wevent) {
  sync_engine_impl.Call(FROM_HERE,
                        &OnPollSyncCycleOnOwnerThread,
                        cb,
                        wevent);
}
#endif

void BraveInit(WeakHandle<SyncEngineImpl> sync_engine_impl,
               SyncManager::InitArgs* args) {
#if BUILDFLAG(ENABLE_BRAVE_SYNC)
  DCHECK(args);
  args->nudge_sync_cycle_delegate_function =
      base::BindRepeating(&OnNudgeSyncCycle,
                          sync_engine_impl);
  args->poll_sync_cycle_delegate_function =
      base::BindRepeating(&OnPollSyncCycle,
                          sync_engine_impl);
#endif
}

}  // namespace syncer

#define BRAVE_SYNC_ENGINE_BACKEND_DO_INITIALIZE \
BraveInit(host_, &args);

#include "../../../../../../components/sync/driver/glue/sync_engine_backend.cc"   // NOLINT
