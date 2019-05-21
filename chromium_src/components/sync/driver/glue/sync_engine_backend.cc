#include "../../../../../../components/sync/driver/glue/sync_engine_backend.cc"

#include "brave/components/brave_sync/jslib_messages.h"

namespace syncer {

void SyncEngineBackend::OnNudgeSyncCycle(
    brave_sync::RecordsListPtr records_list) {
  host_.Call(FROM_HERE,
             &SyncEngineImpl::HandleNudgeSyncCycle,
             base::Passed(&records_list));
}

void SyncEngineBackend::OnPollSyncCycle(GetRecordsCallback cb,
                                          base::WaitableEvent* wevent) {
  host_.Call(FROM_HERE,
             &SyncEngineImpl::HandlePollSyncCycle, cb, wevent);
}

void SyncEngineBackend::DoDispatchGetRecordsCallback(
    GetRecordsCallback cb, std::unique_ptr<RecordsList> records) {
  cb.Run(std::move(records));
}

void SyncEngineBackend::BraveInit(SyncManager::InitArgs* args) {
  DCHECK(args);
  args->nudge_sync_cycle_delegate_function =
    base::BindRepeating(&SyncEngineBackend::OnNudgeSyncCycle,
                        weak_ptr_factory_.GetWeakPtr());
  args->poll_sync_cycle_delegate_function =
    base::BindRepeating(&SyncEngineBackend::OnPollSyncCycle,
                        weak_ptr_factory_.GetWeakPtr());
}

}  // namespace syncer
