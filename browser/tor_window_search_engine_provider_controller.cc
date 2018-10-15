/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor_window_search_engine_provider_controller.h"

#include "brave/browser/search_engine_provider_util.h"
#include "brave/common/pref_names.h"
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
      otr_profile_->GetOriginalProfile()->GetPrefs());

  // Configure previously used provider because guest profile is off the recored
  // profile.
  auto provider_data =
      TemplateURLPrepopulateData::GetPrepopulatedEngine(
          profile->GetPrefs(),
          alternative_search_engine_provider_in_tor_.GetValue());
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
     otr_template_url_service_->GetDefaultSearchProvider()->data().prepopulate_id);
}
