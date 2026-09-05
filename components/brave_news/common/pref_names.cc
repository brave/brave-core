// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/common/pref_names.h"

#include "brave/components/brave_news/common/locales_helper.h"
#include "brave/components/brave_news/common/p3a_pref_names.h"
#include "brave/components/p3a_utils/feature_usage.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave_news {
namespace prefs {

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
  // These prefs are synced as part of the "Settings" (PREFERENCES) sync type.
  registry->RegisterBooleanPref(
      kShouldShowToolbarButton, true,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterBooleanPref(
      kNewTabPageShowToday, IsUserInDefaultEnabledLocale(),
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterBooleanPref(
      kBraveNewsOptedIn, false,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterDictionaryPref(
      kBraveNewsSources, user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterDictionaryPref(
      kBraveNewsChannels, user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterDictionaryPref(
      kBraveNewsDirectFeeds, user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterBooleanPref(
      kBraveNewsOpenArticlesInNewTab, true,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterBooleanPref(kBraveNewsDisabledByPolicy, false);

  brave_news::p3a::prefs::RegisterProfileNewsMetricsPrefs(registry);
}

}  // namespace prefs

bool IsEnabled(PrefService* prefs) {
  if (prefs->IsManagedPreference(prefs::kBraveNewsDisabledByPolicy) &&
      prefs->GetBoolean(prefs::kBraveNewsDisabledByPolicy)) {
    return false;
  }
  return prefs->GetBoolean(prefs::kNewTabPageShowToday) &&
         prefs->GetBoolean(prefs::kBraveNewsOptedIn);
}

}  // namespace brave_news
