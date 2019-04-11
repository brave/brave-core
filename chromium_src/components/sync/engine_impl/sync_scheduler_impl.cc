#include "../../../../../components/sync/engine_impl/sync_scheduler_impl.cc"

#include "brave/components/brave_sync/jslib_messages.h"

namespace syncer {

void SyncSchedulerImpl::OnNudgeSyncCycle(
    brave_sync::RecordsListPtr records_list) {
  DCHECK(nudge_sync_cycle_delegate_function_);
  nudge_sync_cycle_delegate_function_.Run(std::move(records_list));
}

void SyncSchedulerImpl::OnPollSyncCycle(brave_sync::GetRecordsCallback cb,
                                        base::WaitableEvent* wevent) {
  DCHECK(poll_sync_cycle_delegate_function_);
  poll_sync_cycle_delegate_function_.Run(cb, wevent);
}

void SyncSchedulerImpl::SetNudgeAndPollDelegate(
    brave_sync::NudgeSyncCycleDelegate nudge_delegate,
    brave_sync::PollSyncCycleDelegate poll_delegate) {
  nudge_sync_cycle_delegate_function_ = nudge_delegate;
  poll_sync_cycle_delegate_function_ = poll_delegate;
}

void SyncSchedulerImpl::TryBraveSyncCycleJob() {
  SyncCycle cycle(cycle_context_, this);
  if (mode_ != CONFIGURATION_MODE) {
      syncer_->DownloadBraveRecords(&cycle);
  }
}

}  // namespace syncer
