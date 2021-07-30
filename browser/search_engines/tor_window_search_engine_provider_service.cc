/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/search_engines/tor_window_search_engine_provider_service.h"

#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/search_engines_pref_names.h"
#include "components/search_engines/template_url_data_util.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "components/search_engines/template_url_service.h"

TorWindowSearchEngineProviderService::
TorWindowSearchEngineProviderService(Profile* otr_profile)
    : SearchEngineProviderService(otr_profile) {
  DCHECK(otr_profile->IsTor());

  auto provider_data = GetInitialSearchEngineProvider(otr_profile->GetPrefs());
  alternative_search_engine_url_for_tor_ =
      std::make_unique<TemplateURL>(*provider_data);

  ConfigureSearchEngineProvider();

  observation_.Observe(original_template_url_service_);
}

TorWindowSearchEngineProviderService::~TorWindowSearchEngineProviderService() =
    default;

void TorWindowSearchEngineProviderService::Shutdown() {
  SearchEngineProviderService::Shutdown();
  observation_.Reset();
}

void TorWindowSearchEngineProviderService::ConfigureSearchEngineProvider() {
  const bool use_extension_provider = ShouldUseExtensionSearchProvider();
  otr_profile_->GetPrefs()->SetBoolean(prefs::kDefaultSearchProviderByExtension,
                                       use_extension_provider);

  if (use_extension_provider) {
    UseExtensionSearchProvider();
  } else {
    otr_template_url_service_->SetUserSelectedDefaultSearchProvider(
        alternative_search_engine_url_for_tor_.get());
  }
}

void TorWindowSearchEngineProviderService::OnTemplateURLServiceChanged() {
  ConfigureSearchEngineProvider();
}

std::unique_ptr<TemplateURLData>
TorWindowSearchEngineProviderService::GetInitialSearchEngineProvider(
    PrefService* prefs) const {
  std::unique_ptr<TemplateURLData> provider_data =
      TemplateURLPrepopulateData::GetPrepopulatedDefaultSearch(
          otr_profile_->GetPrefs());
  if (provider_data->prepopulate_id ==
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_QWANT) {
    return provider_data;
  }

  return TemplateURLDataFromPrepopulatedEngine(
      TemplateURLPrepopulateData::duckduckgo);
}
