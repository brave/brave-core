/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/search_engines/normal_window_search_engine_provider_service.h"

#include <string>

#include "base/containers/contains.h"
#include "base/functional/bind.h"
#include "brave/browser/search_engines/search_engine_provider_util.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/search_engines_pref_names.h"
#include "components/search_engines/template_url_service.h"

NormalWindowSearchEngineProviderService::
    NormalWindowSearchEngineProviderService(Profile* profile)
    : profile_(profile) {
  private_search_provider_guid_.Init(
      prefs::kSyncedDefaultPrivateSearchProviderGUID, profile_->GetPrefs(),
      base::BindRepeating(
          &NormalWindowSearchEngineProviderService::OnPreferenceChanged,
          base::Unretained(this)));

  auto* service = TemplateURLServiceFactory::GetForProfile(profile_);
  observation_.Observe(service);

  if (service->loaded()) {
    PrepareInitialPrivateSearchProvider();
    return;
  }

  // Using Unretained safe with subscription_.
  template_url_service_subscription_ =
      service->RegisterOnLoadedCallback(base::BindOnce(
          &NormalWindowSearchEngineProviderService::OnTemplateURLServiceLoaded,
          base::Unretained(this), profile));
}

NormalWindowSearchEngineProviderService::
    ~NormalWindowSearchEngineProviderService() = default;

void NormalWindowSearchEngineProviderService::Shutdown() {
  template_url_service_subscription_ = {};
  private_search_provider_guid_.Destroy();
  observation_.Reset();
}

void NormalWindowSearchEngineProviderService::OnTemplateURLServiceLoaded(
    Profile* profile) {
  template_url_service_subscription_ = {};
  PrepareInitialPrivateSearchProvider();
}

void NormalWindowSearchEngineProviderService::
    PrepareInitialPrivateSearchProvider() {
  brave::PrepareDefaultPrivateSearchProviderDataIfNeeded(profile_);
}

void NormalWindowSearchEngineProviderService::OnPreferenceChanged() {
  brave::UpdateDefaultPrivateSearchProviderData(profile_);
}

void NormalWindowSearchEngineProviderService::OnTemplateURLServiceChanged() {
  UpdateSearchSuggestionsDefaultValue();
}

void NormalWindowSearchEngineProviderService::
    UpdateSearchSuggestionsDefaultValue() {
  // As we want to have different default value for search suggestions option,
  // we should update whenever default provider is changed.
  auto* service = TemplateURLServiceFactory::GetForProfile(profile_);
  auto* template_url = service->GetDefaultSearchProvider();
  if (!template_url) {
    return;
  }

  const std::string default_country_code =
      brave_l10n::GetDefaultISOCountryCodeString();
  constexpr std::array<const char*, 11> kSupportedCountries = {
      "IN", "CA", "DE", "FR", "GB", "US", "AT", "ES", "MX", "BR", "AR"};
  const bool search_suggestions_default_value =
      (template_url->prepopulate_id() ==
       TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE) &&
      base::Contains(kSupportedCountries, default_country_code);

  profile_->GetPrefs()->SetDefaultPrefValue(
      prefs::kSearchSuggestEnabled,
      base::Value(search_suggestions_default_value));
}

void NormalWindowSearchEngineProviderService::
    OnTemplateURLServiceShuttingDown() {
  observation_.Reset();
}
