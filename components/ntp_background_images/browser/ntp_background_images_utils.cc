// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/ntp_background_images/browser/ntp_background_images_utils.h"

#include <string>

#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"

namespace ntp_background_images {

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(
      prefs::kNewTabPageCachedSuperReferralComponentInfo);
  registry->RegisterStringPref(
      prefs::kNewTabPageCachedSuperReferralComponentData, std::string());
  registry->RegisterListPref(
      prefs::kNewTabPageCachedSuperReferralFaviconList);
  registry->RegisterBooleanPref(
      prefs::kNewTabPageGetInitialSRComponentInProgress, false);
}

bool IsValidSuperReferralComponentInfo(const base::Value& component_info) {
  if (!component_info.is_dict())
    return false;

  if (!component_info.FindStringKey(kPublicKey))
    return false;
  if (!component_info.FindStringKey(kComponentID))
    return false;
  if (!component_info.FindStringKey(kThemeName))
    return false;

  return true;
}

}  // namespace ntp_background_images
