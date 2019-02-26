#include "../../../../../../components/sync/driver/glue/sync_backend_host_core.cc"

#include "brave/components/brave_sync/jslib_messages.h"

namespace syncer {

void SyncBackendHostCore::OnNudgeSyncCycle(
    brave_sync::RecordsListPtr records_list) {
  host_.Call(FROM_HERE,
             &SyncBackendHostImpl::HandleNudgeSyncCycle,
             base::Passed(&records_list));
}

void SyncBackendHostCore::OnPollSyncCycle(GetRecordsCallback cb,
                                          base::WaitableEvent* wevent) {
  host_.Call(FROM_HERE,
             &SyncBackendHostImpl::HandlePollSyncCycle, cb, wevent);
}

void SyncBackendHostCore::DoDispatchGetRecordsCallback(
    GetRecordsCallback cb, std::unique_ptr<RecordsList> records) {
  cb.Run(std::move(records));
}

void SyncBackendHostCore::BraveInit(SyncManager::InitArgs* args) {
  DCHECK(args);
  args->nudge_sync_cycle_delegate_function =
    base::BindRepeating(&SyncBackendHostCore::OnNudgeSyncCycle,
                        weak_ptr_factory_.GetWeakPtr());
  args->poll_sync_cycle_delegate_function =
    base::BindRepeating(&SyncBackendHostCore::OnPollSyncCycle,
                        weak_ptr_factory_.GetWeakPtr());
}

}  // namespace syncer
