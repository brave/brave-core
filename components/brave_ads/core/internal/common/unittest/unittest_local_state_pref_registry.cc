/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_local_state_pref_registry.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_local_state_pref_registry_util.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/components/l10n/common/prefs.h"

namespace brave_ads {

void RegisterLocalStatePrefs() {
  RegisterLocalStateStringPref(brave_l10n::prefs::kCountryCode,
                               brave_l10n::GetDefaultISOCountryCodeString());
}

}  // namespace brave_ads
