#include "brave/components/brave_sync/brave_sync_prefs.h"

#define BRAVE_PROFILE_SYNC_SERVICE_BUNDLE \
  brave_sync::Prefs::RegisterProfilePrefs(pref_service_.registry());
#include "../../../../../components/sync/driver/profile_sync_service_bundle.cc"
#undef BRAVE_PROFILE_SYNC_SERVICE_BUNDLE
