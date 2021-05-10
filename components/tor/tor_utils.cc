/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/tor_utils.h"

#include <string>

#include "base/files/file_path.h"
#include "brave/components/tor/tor_constants.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace tor {

void MigrateLastUsedProfileFromLocalStatePrefs(PrefService* local_state) {
  // Do this for legacy tor profile migration because tor profile might be last
  // active profile before upgrading
  std::string last_used_profile_name =
      local_state->GetString(prefs::kProfileLastUsed);
  if (!last_used_profile_name.empty() &&
      last_used_profile_name ==
          base::FilePath(tor::kTorProfileDir).AsUTF8Unsafe()) {
    local_state->SetString(prefs::kProfileLastUsed, chrome::kInitialProfile);
  }
}

}  // namespace tor
