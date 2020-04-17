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

#define BRAVE_PROFILE_SYNC_SERVICE            \
  auth_manager_->CreateAccessTokenFetcher(    \
      url_loader_factory_, sync_service_url_);

#define BRAVE_START_UP_SLOW_ENGINE_COMPONENTS     \
    auth_manager_->DeriveSigningKeys(             \
      sync_client_->GetPrefService()->GetString(  \
        brave_sync::prefs::kSyncSeed));

#define BRAVE_ON_FIRST_SETUP_COMPLETE_PREF_CHANGE   \
    if (is_first_setup_complete)                    \
      auth_manager_->DeriveSigningKeys(             \
        sync_client_->GetPrefService()->GetString(  \
          brave_sync::prefs::kSyncSeed));

#define BRAVE_ON_ENGINE_INITIALIZED                                           \
  std::string sync_code =                                                     \
    sync_client_->GetPrefService()->GetString(brave_sync::prefs::kSyncSeed);  \
  if (!sync_code.empty()) {                                                   \
    GetUserSettings()->EnableEncryptEverything();                             \
    if (GetUserSettings()->IsPassphraseRequired()) {                          \
      if (!GetUserSettings()->SetDecryptionPassphrase(sync_code))             \
      LOG(ERROR) << "Set decryption passphrase failed";                       \
    } else {                                                                  \
      if (!GetUserSettings()->IsUsingSecondaryPassphrase())                   \
      GetUserSettings()->SetEncryptionPassphrase(sync_code);                  \
    }                                                                         \
  }

#define BRAVE_STOP_IMPL \
  auth_manager_->ResetKeys();

#include "../../../../../components/sync/driver/profile_sync_service.cc"
#undef BRAVE_SET_POLL_INTERVAL
#undef BRAVE_IS_SIGNED_IN
#undef BRAVE_PROFILE_SYNC_SERVICE
#undef BRAVE_START_UP_SLOW_ENGINE_COMPONENTS
#undef BRAVE_ON_FIRST_SETUP_COMPLETE_PREF_CHANGE
#undef BRAVE_ON_ENGINE_INITIALIZED
#undef BRAVE_STOP_IMPL
