/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor_window_search_engine_provider_controller.h"

#include "brave/browser/search_engine_provider_util.h"
#include "brave/common/pref_names.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "chrome/browser/profiles/profile.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "components/search_engines/template_url_service.h"

TorWindowSearchEngineProviderController::
TorWindowSearchEngineProviderController(Profile* profile)
    : SearchEngineProviderControllerBase(profile) {
  DCHECK(profile->IsTorProfile());
  DCHECK_EQ(profile->GetProfileType(), Profile::GUEST_PROFILE);

  alternative_search_engine_provider_in_tor_.Init(
      kAlternativeSearchEngineProviderInTor,
      profile->GetOriginalProfile()->GetPrefs());

  // Configure previously used provider because effective tor profile is
  // off the recored profile.
  auto provider_data =
      TemplateURLPrepopulateData::GetPrepopulatedEngine(
          profile->GetPrefs(), GetInitialSearchEngineProvider());
  TemplateURL provider_url(*provider_data);
  otr_template_url_service_->SetUserSelectedDefaultSearchProvider(
      &provider_url);

  // Monitor otr(off the record) profile's search engine changing to caching
  // in original profile.
  otr_template_url_service_->AddObserver(this);
}

TorWindowSearchEngineProviderController::
~TorWindowSearchEngineProviderController() {
  otr_template_url_service_->RemoveObserver(this);
}

void TorWindowSearchEngineProviderController::OnTemplateURLServiceChanged() {
  alternative_search_engine_provider_in_tor_.SetValue(
     otr_template_url_service_->GetDefaultSearchProvider()->
         data().prepopulate_id);
}

int TorWindowSearchEngineProviderController::
GetInitialSearchEngineProvider() const {
  int initial_id = alternative_search_engine_provider_in_tor_.GetValue();

  bool region_for_qwant =
      TemplateURLPrepopulateData::GetPrepopulatedDefaultSearch(
          otr_profile_->GetPrefs())->prepopulate_id ==
          TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_QWANT;

  // If this is first run, |initial_id| is invalid. Then, use qwant or ddg
  // depends on default prepopulate data.
  if (initial_id ==
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_INVALID) {
    initial_id = region_for_qwant ?
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_QWANT :
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO;
  }

  return initial_id;
}
