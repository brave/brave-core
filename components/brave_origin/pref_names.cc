/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/pref_names.h"

#include "components/prefs/pref_registry_simple.h"

namespace brave_origin::prefs {

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
#if BUILDFLAG(IS_ANDROID)
  registry->RegisterBooleanPref(kBraveOriginSubscriptionActiveAndroid, false);
  registry->RegisterStringPref(kBraveOriginPurchaseTokenAndroid, "");
  registry->RegisterStringPref(kBraveOriginPackageNameAndroid, "");
  registry->RegisterStringPref(kBraveOriginProductIdAndroid, "");
  registry->RegisterStringPref(kBraveOriginOrderIdAndroid, "");
  // 0 is not linked
  registry->RegisterIntegerPref(kBraveOriginSubscriptionLinkStatusAndroid, 0);
#endif
}

}  // namespace brave_origin::prefs
