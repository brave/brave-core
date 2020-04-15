#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_sync/crypto/crypto.h"

#define BRAVE_REGISTER_MESSAGES                              \
  web_ui()->RegisterMessageCallback(                         \
      "SyncSetupSetSyncCode",                                \
      base::BindRepeating(&PeopleHandler::HandleSetSyncCode, \
                          base::Unretained(this)));          \
  web_ui()->RegisterMessageCallback(                         \
      "SyncSetupGetSyncCode",                                \
      base::BindRepeating(&PeopleHandler::HandleGetSyncCode, \
                          base::Unretained(this)));

// Used for override DisableReasons in ProfileSyncService
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
  return (current_url == chrome::GetSettingsUrl("braveSync/setup"));

#include "../../../../../../../chrome/browser/ui/webui/settings/people_handler.cc"
#undef BRAVE_REGISTER_MESSAGES
#undef BRAVE_HANDLE_SHOW_SETUP_UI
#undef BRAVE_CLOSE_SYNC_SETUP
#undef BRAVE_IS_SYNC_SUBPAGE

namespace settings {

void PeopleHandler::HandleGetSyncCode(const base::ListValue* args) {
  AllowJavascript();

  CHECK_EQ(1U, args->GetSize());
  const base::Value* callback_id;
  CHECK(args->Get(0, &callback_id));

  std::string sync_code =
      profile_->GetPrefs()->GetString(brave_sync::prefs::kSyncSeed);
  if (sync_code.empty()) {
    std::vector<uint8_t> seed = brave_sync::crypto::GetSeed();
    sync_code = brave_sync::crypto::PassphraseFromBytes32(seed);
  }

  ResolveJavascriptCallback(*callback_id, base::Value(sync_code));
}

void PeopleHandler::HandleSetSyncCode(const base::ListValue* args) {
  CHECK_EQ(1U, args->GetSize());
  const base::Value* sync_code;
  CHECK(args->Get(0, &sync_code));

  std::vector<uint8_t> seed;
  if (!brave_sync::crypto::PassphraseToBytes32(sync_code->GetString(), &seed)) {
    LOG(ERROR) << "invalid sync code";
    return;
  }
  profile_->GetPrefs()->SetString(brave_sync::prefs::kSyncSeed,
                                  sync_code->GetString());
}
}  // namespace settings
