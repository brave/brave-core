/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/search_engines/tor_window_search_engine_provider_service.h"

#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "chrome/browser/profiles/profile.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "components/search_engines/template_url_service.h"

TorWindowSearchEngineProviderService::
TorWindowSearchEngineProviderService(Profile* otr_profile)
    : SearchEngineProviderService(otr_profile) {
  DCHECK(otr_profile->IsTor());

  // Config default provider for tor window.
  auto provider_data = GetInitialSearchEngineProvider(otr_profile->GetPrefs());
  TemplateURL provider_url(*provider_data);
  otr_template_url_service_->SetUserSelectedDefaultSearchProvider(
      &provider_url);
}

TorWindowSearchEngineProviderService::~TorWindowSearchEngineProviderService() =
    default;

std::unique_ptr<TemplateURLData>
TorWindowSearchEngineProviderService::GetInitialSearchEngineProvider(
    PrefService* prefs) const {
  std::unique_ptr<TemplateURLData> provider_data;

  int initial_id = TemplateURLPrepopulateData::GetPrepopulatedDefaultSearch(
                       otr_profile_->GetPrefs())
                       ->prepopulate_id;
  switch (initial_id) {
    case TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_QWANT:
    case TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO:
    case TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO_DE:
    case TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE:
      break;

    default:
      initial_id =
          TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO;
      break;
  }
  provider_data =
      TemplateURLPrepopulateData::GetPrepopulatedEngine(prefs, initial_id);

  DCHECK(provider_data);
  return provider_data;
}
