/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/brave_sync_prefs.h"
#define BRAVE_SIGN_IN_PRIMARY_ACCOUNT                         \
  const char sync_code[] =                                    \
      "badge unique kiwi orient spring venue piano "          \
      "lake admit ill roof brother grant hour better "        \
      "proud cabbage fee slow economy wage final fox cancel"; \
  brave_sync::Prefs brave_sync_prefs(profile_->GetPrefs());   \
  brave_sync_prefs.SetSyncV1Migrated(true);                   \
  brave_sync_prefs.SetSeed(sync_code);

#define BRAVE_SIGN_OUT_PRIMARY_ACCOUNT                      \
  brave_sync::Prefs brave_sync_prefs(profile_->GetPrefs()); \
  brave_sync_prefs.Clear();
#include "../../../../../../../chrome/browser/sync/test/integration/profile_sync_service_harness.cc"
#undef BRAVE_SIGN_IN_PRIMARY_ACCOUNT
#undef BRAVE_SIGN_OUT_PRIMARY_ACCOUNT
