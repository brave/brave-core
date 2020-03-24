// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/ntp_background_images/browser/ntp_background_images_utils.h"

#include <string>

#include "brave/components/ntp_background_images/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"

namespace ntp_background_images {

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterStringPref(prefs::kNewTabPageCachedReferralPromoCode,
                               std::string());
  registry->RegisterDictionaryPref(
      prefs::kNewTabPageCachedSuperReferrerComponentInfo);
}

}  // namespace ntp_background_images
