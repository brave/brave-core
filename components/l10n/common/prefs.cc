/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/l10n/common/prefs.h"

#include <string>
#include "brave/components/l10n/common/locale_util.h"
#include "components/prefs/pref_registry_simple.h"

namespace brave_l10n {

void RegisterL10nLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterStringPref(prefs::kCountryCode,
                               GetDefaultISOCountryCodeString());
}

}  // namespace brave_l10n
