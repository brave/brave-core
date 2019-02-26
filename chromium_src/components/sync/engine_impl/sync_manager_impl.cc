#include "../../../../../components/sync/engine_impl/sync_manager_impl.cc"
#include "brave/components/brave_sync/jslib_messages.h"

namespace syncer {

void SyncManagerImpl::BraveInit(InitArgs* args) {
  DCHECK(args);
  scheduler_->SetNudgeAndPollDelegate(args->nudge_sync_cycle_delegate_function,
                                      args->poll_sync_cycle_delegate_function);
}

}  // namespace syncer
