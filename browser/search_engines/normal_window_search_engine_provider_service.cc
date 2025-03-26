/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/search_engines/normal_window_search_engine_provider_service.h"

#include <string>

#include "base/functional/bind.h"
#include "brave/browser/search_engines/pref_names.h"
#include "brave/browser/search_engines/search_engine_provider_util.h"
#include "brave/components/l10n/common/country_code_util.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/common/pref_names.h"
#include "components/country_codes/country_codes.h"
#include "components/search_engines/search_engines_pref_names.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "components/search_engines/template_url_service.h"

NormalWindowSearchEngineProviderService::
    NormalWindowSearchEngineProviderService(Profile* profile)
    : profile_(profile) {
  brave::UpdateDefaultSearchSuggestionsPrefs(*g_browser_process->local_state(),
                                             *profile_->GetPrefs());
  private_search_provider_guid_.Init(
      prefs::kSyncedDefaultPrivateSearchProviderGUID, profile_->GetPrefs(),
      base::BindRepeating(
          &NormalWindowSearchEngineProviderService::OnPreferenceChanged,
          base::Unretained(this)));

  auto* service = TemplateURLServiceFactory::GetForProfile(profile_);
  if (service->loaded()) {
    OnTemplateURLServiceLoaded();
    return;
  }

  // Using Unretained safe with subscription_.
  template_url_service_subscription_ =
      service->RegisterOnLoadedCallback(base::BindOnce(
          &NormalWindowSearchEngineProviderService::OnTemplateURLServiceLoaded,
          base::Unretained(this)));
}

NormalWindowSearchEngineProviderService::
    ~NormalWindowSearchEngineProviderService() = default;

void NormalWindowSearchEngineProviderService::Shutdown() {
  template_url_service_subscription_ = {};
  private_search_provider_guid_.Destroy();
}

void NormalWindowSearchEngineProviderService::OnTemplateURLServiceLoaded() {
  template_url_service_subscription_ = {};
  PrepareInitialPrivateSearchProvider();
  MigrateSearchEnginePrefsInJP();
}

void NormalWindowSearchEngineProviderService::
    PrepareInitialPrivateSearchProvider() {
  brave::PrepareDefaultPrivateSearchProviderDataIfNeeded(*profile_);
}

void NormalWindowSearchEngineProviderService::OnPreferenceChanged() {
  brave::UpdateDefaultPrivateSearchProviderData(*profile_);
}

void NormalWindowSearchEngineProviderService::MigrateSearchEnginePrefsInJP() {
  auto* prefs = profile_->GetPrefs();
  if (prefs->GetBoolean(kMigratedSearchDefaultInJP)) {
    return;
  }

  prefs->SetBoolean(kMigratedSearchDefaultInJP, true);

  const std::string country_string =
      brave_l10n::GetCountryCode(g_browser_process->local_state());
  if (country_string != "JP") {
    return;
  }

  auto* preference =
      prefs->FindPreference(prefs::kSyncedDefaultSearchProviderGUID);
  if (preference->HasUserSetting()) {
    return;
  }

  auto* service = TemplateURLServiceFactory::GetForProfile(profile_);
  if (!service->loaded()) {
    return;
  }

  if (service->GetDefaultSearchProvider()->prepopulate_id() ==
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YAHOO_JP) {
    return;
  }

  auto data = TemplateURLPrepopulateData::GetPrepopulatedEngine(
      *prefs, country_codes::CountryId(country_string),
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_YAHOO_JP);
  if (!data) {
    return;
  }

  TemplateURL url(*data);
  service->SetUserSelectedDefaultSearchProvider(&url);

  if (prefs->GetBoolean(prefs::kSearchSuggestEnabled)) {
    prefs->SetBoolean(prefs::kSearchSuggestEnabled, false);
  }
}
