#include "brave/common/webui_url_constants.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_sync/crypto/crypto.h"

#define BRAVE_REGISTER_MESSAGES                                          \
  web_ui()->RegisterMessageCallback(                                     \
      "SyncSetupSetSyncCode",                                            \
      base::BindRepeating(&PeopleHandler::HandleSetSyncCode,             \
                          base::Unretained(this)));                      \
  web_ui()->RegisterMessageCallback(                                     \
      "SyncSetupGetSyncCode",                                            \
      base::BindRepeating(&PeopleHandler::HandleGetSyncCode,             \
                          base::Unretained(this)));                      \
  web_ui()->RegisterMessageCallback(                                     \
      "SyncSetupReset", base::BindRepeating(&PeopleHandler::HandleReset, \
                                            base::Unretained(this)));

#define BRAVE_HANDLE_SHOW_SETUP_UI \
  profile_->GetPrefs()->SetBoolean(brave_sync::prefs::kSyncEnabled, true);

#define BRAVE_CLOSE_SYNC_SETUP                                                \
  syncer::SyncService* sync_service = GetSyncService();                       \
  if (sync_service &&                                                         \
      !sync_service->GetUserSettings()->IsFirstSetupComplete()) {             \
    DVLOG(1) << "Sync setup aborted by user action";                          \
    sync_service->StopAndClear();                                             \
    profile_->GetPrefs()->SetBoolean(brave_sync::prefs::kSyncEnabled, false); \
  }

#define BRAVE_IS_SYNC_SUBPAGE \
  return (current_url == chrome::GetSettingsUrl(kBraveSyncSetupPath));

// IsSetupInProgress isn't accurate in brave sync flow especially for the first
// time setup, we rely on it to display setup dialog
#define BRAVE_GET_SYNC_STATUS_DICTIONARY  \
  sync_status->SetBoolean(                \
      "firstSetupInProgress",             \
      service && !disallowed_by_policy && \
          !service->GetUserSettings()->IsFirstSetupComplete());

#include "../../../../../../../chrome/browser/ui/webui/settings/people_handler.cc"
#undef BRAVE_REGISTER_MESSAGES
#undef BRAVE_HANDLE_SHOW_SETUP_UI
#undef BRAVE_CLOSE_SYNC_SETUP
#undef BRAVE_IS_SYNC_SUBPAGE
#undef BRAVE_GET_SYNC_STATUS_DICTIONARY

namespace settings {

void PeopleHandler::HandleGetSyncCode(const base::ListValue* args) {
  AllowJavascript();

  CHECK_EQ(1U, args->GetSize());
  const base::Value* callback_id;
  CHECK(args->Get(0, &callback_id));

  brave_sync::prefs::Prefs brave_sync_prefs(profile_->GetPrefs());
  std::string sync_code =
    brave_sync_prefs.GetSeed();
  if (sync_code.empty()) {
    std::vector<uint8_t> seed = brave_sync::crypto::GetSeed();
    sync_code = brave_sync::crypto::PassphraseFromBytes32(seed);
  }

  ResolveJavascriptCallback(*callback_id, base::Value(sync_code));
}

void PeopleHandler::HandleSetSyncCode(const base::ListValue* args) {
  CHECK_EQ(2U, args->GetSize());
  const base::Value* callback_id;
  CHECK(args->Get(0, &callback_id));
  const base::Value* sync_code;
  CHECK(args->Get(1, &sync_code));

  std::vector<uint8_t> seed;
  if (!brave_sync::crypto::PassphraseToBytes32(sync_code->GetString(), &seed)) {
    LOG(ERROR) << "invalid sync code:" << sync_code->GetString();
    ResolveJavascriptCallback(*callback_id, base::Value(false));
    return;
  }
  brave_sync::prefs::Prefs brave_sync_prefs(profile_->GetPrefs());
  if (!brave_sync_prefs.SetSeed(sync_code->GetString())) {
    ResolveJavascriptCallback(*callback_id, base::Value(false));
    return;
  }
  ResolveJavascriptCallback(*callback_id, base::Value(true));
}

void PeopleHandler::HandleReset(const base::ListValue* args) {

  syncer::SyncService* sync_service = GetSyncService();
  if (sync_service) {
    sync_service->GetUserSettings()->SetSyncRequested(false);
    sync_service->StopAndClear();
  }
  brave_sync::prefs::Prefs brave_sync_prefs(profile_->GetPrefs());
  brave_sync_prefs.Clear();

  // Sync prefs will be clear in ProfileSyncService::StopImpl
}
}  // namespace settings
