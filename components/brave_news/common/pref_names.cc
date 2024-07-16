// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/common/pref_names.h"

#include "brave/components/brave_news/common/locales_helper.h"
#include "brave/components/brave_news/common/p3a_pref_names.h"
#include "brave/components/p3a_utils/feature_usage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave_news {
namespace prefs {

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kShouldShowToolbarButton, true);
  registry->RegisterBooleanPref(kNewTabPageShowToday,
                                IsUserInDefaultEnabledLocale());
  registry->RegisterBooleanPref(kBraveNewsOptedIn, false);
  registry->RegisterDictionaryPref(kBraveNewsSources);
  registry->RegisterDictionaryPref(kBraveNewsChannels);
  registry->RegisterDictionaryPref(kBraveNewsDirectFeeds);
  registry->RegisterBooleanPref(kBraveNewsOpenArticlesInNewTab, true);

  brave_news::p3a::prefs::RegisterProfileNewsMetricsPrefs(registry);
}

}  // namespace prefs

bool IsEnabled(PrefService* prefs) {
  return prefs->GetBoolean(prefs::kNewTabPageShowToday) &&
         prefs->GetBoolean(prefs::kBraveNewsOptedIn);
}

}  // namespace brave_news
