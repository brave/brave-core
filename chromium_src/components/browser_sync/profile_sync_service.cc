// For use_transport_only_mode
#define IsSyncFeatureEnabled IsBraveSyncEnabled
#include "../../../../components/browser_sync/profile_sync_service.cc"

#include "base/bind.h"
#include "brave/components/brave_sync/jslib_messages.h"
#include "chrome/browser/sync/chrome_sync_client.h"
#include "components/sync/engine/sync_credentials.h"
#include "content/public/browser/browser_thread.h"

namespace syncer {
const int64_t kBraveDefaultShortPollIntervalSeconds = 60;
const int64_t kBraveDefaultLongPollIntervalSeconds = 90;
}

namespace browser_sync {

namespace {

syncer::SyncCredentials GetDummyCredentials() {
  syncer::SyncCredentials credentials;
  credentials.account_id = "dummy_account_id";
  credentials.email = "dummy_email";
  credentials.sync_token = "dummy_access_token";
  return credentials;
}

}   // namespace

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

  params->credentials = GetDummyCredentials();

  sync_prefs_.SetShortPollInterval(
    base::TimeDelta::FromSeconds(
      syncer::kBraveDefaultShortPollIntervalSeconds));
  sync_prefs_.SetLongPollInterval(
    base::TimeDelta::FromSeconds(
      syncer::kBraveDefaultLongPollIntervalSeconds));
}

void ProfileSyncService::OnNudgeSyncCycle(
    brave_sync::RecordsListPtr records_list) {}

void ProfileSyncService::OnPollSyncCycle(brave_sync::GetRecordsCallback cb,
                                         base::WaitableEvent* wevent) {}

bool ProfileSyncService::IsBraveSyncEnabled() const {
  return false;
}

}   // namespace browser_sync
