/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/search_engines/tor_window_search_engine_provider_service.h"

#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/search_engines/search_engine_provider_util.h"
#include "brave/common/pref_names.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "chrome/browser/profiles/profile.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "components/search_engines/template_url_service.h"

TorWindowSearchEngineProviderService::
TorWindowSearchEngineProviderService(Profile* otr_profile)
    : SearchEngineProviderService(otr_profile) {
  DCHECK(brave::IsTorProfile(otr_profile));
  DCHECK(otr_profile->IsOffTheRecord());

  alternative_search_engine_provider_in_tor_.Init(
      kAlternativeSearchEngineProviderInTor,
      otr_profile->GetOriginalProfile()->GetPrefs());

  // Configure previously used provider because effective tor profile is
  // off the recored profile.
  auto provider_data = GetInitialSearchEngineProvider(otr_profile->GetPrefs());
  TemplateURL provider_url(*provider_data);
  otr_template_url_service_->SetUserSelectedDefaultSearchProvider(
      &provider_url);

  // Monitor otr(off the record) profile's search engine changing to caching
  // in original profile.
  otr_template_url_service_->AddObserver(this);
}

TorWindowSearchEngineProviderService::
~TorWindowSearchEngineProviderService() {
  otr_template_url_service_->RemoveObserver(this);
}

void TorWindowSearchEngineProviderService::OnTemplateURLServiceChanged() {
  alternative_search_engine_provider_in_tor_.SetValue(
     otr_template_url_service_->GetDefaultSearchProvider()->
         data().prepopulate_id);
}

std::unique_ptr<TemplateURLData>
TorWindowSearchEngineProviderService::GetInitialSearchEngineProvider(
    PrefService* prefs) const {
  std::unique_ptr<TemplateURLData> provider_data = nullptr;
  int initial_id = alternative_search_engine_provider_in_tor_.GetValue();
  if (initial_id !=
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_INVALID) {
    provider_data = std::move(
        TemplateURLPrepopulateData::GetPrepopulatedEngine(prefs, initial_id));
  }

  // If this is first run, |initial_id| is invalid. Then, use qwant or ddg
  // depends on default prepopulate data. If not, check that the initial_id
  // returned data.
  if (!provider_data) {
    initial_id = TemplateURLPrepopulateData::GetPrepopulatedDefaultSearch(
                     otr_profile_->GetPrefs())
                     ->prepopulate_id;
    switch (initial_id) {
      case TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_QWANT:
      case TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO:
      case TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO_DE:
      case TemplateURLPrepopulateData::
          PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE:
        break;

      default:
        initial_id =
            TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO;
        break;
    }
    provider_data = std::move(
        TemplateURLPrepopulateData::GetPrepopulatedEngine(prefs, initial_id));
  }

  DCHECK(provider_data);
  return provider_data;
}
