#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_sync/fake_access_token_fetcher.h"

#define BRAVE_PROFILE_SYNC_SERVICE_BUNDLE \
  brave_sync::Prefs::RegisterProfilePrefs(pref_service_.registry());
#define BRAVE_CREATE_BASIC_INIT_PARAMS        \
  init_params.access_token_fetcher_for_test = \
      std::make_unique<brave_sync::FakeAccessTokenFetcher>(nullptr);
#include "../../../../../components/sync/driver/profile_sync_service_bundle.cc"
#undef BRAVE_PROFILE_SYNC_SERVICE_BUNDLE
#undef BRAVE_CREATE_BASIC_INIT_PARAMS
