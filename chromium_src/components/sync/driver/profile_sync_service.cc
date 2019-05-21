// For use_transport_only_mode
#define IsSyncFeatureEnabled IsBraveSyncEnabled
#include "../../../../components/sync/driver/profile_sync_service.cc"

#include "base/bind.h"
#include "brave/components/brave_sync/jslib_messages.h"
#include "chrome/browser/sync/chrome_sync_client.h"
#include "content/public/browser/browser_thread.h"

namespace syncer {
const int64_t kBraveDefaultPollIntervalSeconds = 60;

syncer::SyncClient* ProfileSyncService::GetSyncClient() {
  DCHECK(sync_client_);
  return sync_client_.get();
}

syncer::SyncEngine* ProfileSyncService::GetSyncEngine() {
  DCHECK(engine_);
  return engine_.get();
}

void ProfileSyncService::BraveEngineParamsInit(
    syncer::SyncEngine::InitParams* params) {
  DCHECK(params);
  params->nudge_sync_cycle_delegate_function =
      base::BindRepeating(&ProfileSyncService::OnNudgeSyncCycle,
                          sync_enabled_weak_factory_.GetWeakPtr());
  params->poll_sync_cycle_delegate_function =
      base::BindRepeating(&ProfileSyncService::OnPollSyncCycle,
                          sync_enabled_weak_factory_.GetWeakPtr());

  sync_prefs_.SetPollInterval(
    base::TimeDelta::FromSeconds(
      syncer::kBraveDefaultPollIntervalSeconds));
}

void ProfileSyncService::OnNudgeSyncCycle(
    brave_sync::RecordsListPtr records_list) {}

void ProfileSyncService::OnPollSyncCycle(brave_sync::GetRecordsCallback cb,
                                         base::WaitableEvent* wevent) {}

bool ProfileSyncService::IsBraveSyncEnabled() const {
  return false;
}

}   // namespace syncer
