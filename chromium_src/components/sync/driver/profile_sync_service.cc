#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "components/prefs/pref_service.h"

namespace syncer {
const int64_t kBraveDefaultPollIntervalSeconds = 60;
}  // namespace syncer

#define BRAVE_SET_POLL_INTERVAL \
  sync_prefs_.SetPollInterval(  \
      base::TimeDelta::FromSeconds(syncer::kBraveDefaultPollIntervalSeconds));

#define BRAVE_IS_SIGNED_IN                           \
  return sync_client_->GetPrefService()->GetBoolean( \
      brave_sync::prefs::kSyncEnabled);

#define BRAVE_GET_AUTHENTICATED_ACCOUNT_INFO                               \
  AccountInfo account_info;                                                \
  account_info.account_id = CoreAccountId::FromString("dummy_account_id"); \
  return std::move(account_info);

#include "../../../../../components/sync/driver/profile_sync_service.cc"
