#include "../../../../../../components/sync/driver/glue/sync_backend_host_impl.cc"

#include "brave/components/brave_sync/jslib_messages.h"

namespace syncer {

void SyncBackendHostImpl::HandleNudgeSyncCycle(
    brave_sync::RecordsListPtr records_list) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(nudge_sync_cycle_delegate_function_);
  nudge_sync_cycle_delegate_function_.Run(std::move(records_list));
}

void SyncBackendHostImpl::HandlePollSyncCycle(
    GetRecordsCallback cb,
    base::WaitableEvent* wevent) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(poll_sync_cycle_delegate_function_);
  poll_sync_cycle_delegate_function_.Run(cb, wevent);
}

void SyncBackendHostImpl::DispatchGetRecordsCallback(
    GetRecordsCallback cb, std::unique_ptr<RecordsList> records) {
  sync_task_runner_->PostTask(
    FROM_HERE,
    base::BindOnce(&SyncBackendHostCore::DoDispatchGetRecordsCallback, core_,
                   cb, std::move(records)));
}

}  // namespacd syncer
