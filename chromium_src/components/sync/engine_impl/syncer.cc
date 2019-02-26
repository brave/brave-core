#include "brave/components/brave_sync/jslib_messages.h"
#include "base/synchronization/waitable_event.h"
#include "../../../../../components/sync/engine_impl/syncer.cc"

namespace syncer {

using brave_sync::GetRecordsCallback;
using brave_sync::RecordsList;

void Syncer::OnGetRecords(std::unique_ptr<RecordsList> records) {
  brave_records_ = std::move(records);
}

void Syncer::DownloadBraveRecords(SyncCycle* cycle) {
  // syncer will be alive as long as sync is enabled
  brave_records_.reset();
  brave_sync::GetRecordsCallback on_get_records =
    base::BindRepeating(&Syncer::OnGetRecords, AsWeakPtr());
  base::WaitableEvent wevent;
  cycle->delegate()->OnPollSyncCycle(on_get_records, &wevent);
  // Make sure OnGetRecords will be the next task on sync thread
  wevent.Wait();
}

}  // namespace syncer

