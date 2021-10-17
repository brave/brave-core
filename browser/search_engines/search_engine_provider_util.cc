/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/search_engines/search_engine_provider_util.h"

#include "brave/common/pref_names.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/search_engines/template_url_data.h"
#include "components/search_engines/template_url_prepopulate_data.h"

namespace brave {

bool UseAlternativeSearchEngineProviderEnabled(Profile* profile) {
  return profile->GetOriginalProfile()->GetPrefs()->GetBoolean(
      kUseAlternativeSearchEngineProvider);
}

bool ShowAlternativeSearchEngineProviderToggleEnabled(Profile* profile) {
  return profile->GetOriginalProfile()->GetPrefs()->GetBoolean(
      kShowAlternativeSearchEngineProviderToggle);
}

void ToggleUseAlternativeSearchEngineProvider(Profile* profile) {
  if (brave::IsRegionForQwant(profile))
    return;

  profile->GetOriginalProfile()->GetPrefs()->SetBoolean(
      kUseAlternativeSearchEngineProvider,
      !UseAlternativeSearchEngineProviderEnabled(profile));
}

void SetShowAlternativeSearchEngineProviderToggle(Profile* profile) {
  if (brave::IsRegionForQwant(profile))
    return;

  profile->GetOriginalProfile()->GetPrefs()->SetBoolean(
      kShowAlternativeSearchEngineProviderToggle, true);
}

bool IsRegionForQwant(Profile* profile) {
  return TemplateURLPrepopulateData::GetPrepopulatedDefaultSearch(
      profile->GetPrefs())->prepopulate_id ==
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_QWANT;
}

}  // namespace brave
