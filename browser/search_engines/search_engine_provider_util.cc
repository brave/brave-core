/* This Source Code Form is subject to the terms of the Mozilla Public
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

void ToggleUseAlternativeSearchEngineProvider(Profile* profile) {
  if (brave::IsRegionForQwant(profile))
    return;

  profile->GetOriginalProfile()->GetPrefs()->SetBoolean(
      kUseAlternativeSearchEngineProvider,
      !UseAlternativeSearchEngineProviderEnabled(profile));
}

bool IsRegionForQwant(Profile* profile) {
  return TemplateURLPrepopulateData::GetPrepopulatedDefaultSearch(
      profile->GetPrefs())->prepopulate_id ==
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_QWANT;
}

bool IsOTRGuestProfile(Profile* profile) {
  // Note, that in search_engine_provider_service_factory.cc and
  // guest_window_search_engine_provider_service.cc we check if this is an OTR
  // guest profile before OffTheRecordProfileImpl::Init() had a chance to mark
  // this profile as guest. Therefore, we go to the regular profile of this OTR
  // profile to check for guest.
  DCHECK(profile);
  return (profile->HasOffTheRecordProfile() &&
    profile->GetOffTheRecordProfile() == profile &&
    profile->GetOriginalProfile()->IsGuestSession());
}

}  // namespace brave
