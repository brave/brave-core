/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/l10n/common/region_code_util.h"

#include "brave/components/l10n/common/locale_util.h"
#include "brave/components/l10n/common/prefs.h"
#include "brave/components/l10n/common/region_code_feature.h"
#include "components/prefs/pref_service.h"

namespace brave_l10n {

std::string GetCurrentGeoRegionCode(const PrefService* local_state) {
  if (base::FeatureList::IsEnabled(kFetchResourcesByRegionCodeFeature)) {
    return local_state->GetString(brave_l10n::prefs::kGeoRegionCode);
  }
  return GetDefaultISOCountryCodeString();
}

}  // namespace brave_l10n
