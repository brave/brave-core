/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/search_engines/search_engine_provider_service.h"

#include "brave/browser/search_engines/search_engine_provider_util.h"
#include "brave/common/pref_names.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/prepopulated_engines.h"
#include "components/search_engines/template_url_prepopulate_data.h"

SearchEngineProviderService::SearchEngineProviderService(
    Profile* otr_profile)
    : otr_profile_(otr_profile),
      template_url_service_(
          TemplateURLServiceFactory::GetForProfile(otr_profile_)) {
  use_alternative_search_engine_provider_.Init(
      kUseAlternativeSearchEngineProvider,
      otr_profile_->GetOriginalProfile()->GetPrefs(),
      base::Bind(&SearchEngineProviderService::OnPreferenceChanged,
                 base::Unretained(this)));

  std::vector<TemplateURLPrepopulateData::BravePrepopulatedEngineID>
      alt_search_providers = {
          TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO,
          TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO_DE,
          TemplateURLPrepopulateData::
              PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE
      };

  std::unique_ptr<TemplateURLData> data;
  for (const auto& id : alt_search_providers) {
    data = TemplateURLPrepopulateData::GetPrepopulatedEngine(
        otr_profile->GetPrefs(), id);
    if (data)
      break;
  }

  // There should ALWAYS be one entry
  DCHECK(data);
  alternative_search_engine_url_.reset(new TemplateURL(*data));
  observation_.Observe(template_url_service_);
}

SearchEngineProviderService::~SearchEngineProviderService() = default;

void SearchEngineProviderService::OnTemplateURLServiceChanged() {
  // When this change is came from pref changing, we don't need to control
  // prefs again.
  if (ignore_template_url_service_changing_)
    return;

  // The purpose of below code is toggling alternative prefs
  // when user changes from ddg to different search engine provider
  // (or vice versa) from settings ui.
  bool is_ddg_is_set = false;
  switch (template_url_service_->GetDefaultSearchProvider()->data()
              .prepopulate_id) {
    case TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO:
    case TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO_DE:
    case TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE:
      is_ddg_is_set = true;
      break;
    default:
      break;
  }

  if (UseAlternativeSearchEngineProvider() || is_ddg_is_set)
    brave::ToggleUseAlternativeSearchEngineProvider(otr_profile_);
}

void SearchEngineProviderService::OnPreferenceChanged(
    const std::string& pref_name) {
  DCHECK(pref_name == kUseAlternativeSearchEngineProvider);

  // When this call is from setting's change, we don't need to set provider
  // again.
  if (ignore_template_url_service_changing_)
    return;

  base::AutoReset<bool> reset(&ignore_template_url_service_changing_, true);
  UseAlternativeSearchEngineProvider()
      ? ChangeToAlternativeSearchEngineProvider()
      : ChangeToNormalWindowSearchEngineProvider();
}

bool
SearchEngineProviderService::UseAlternativeSearchEngineProvider() const {
  return use_alternative_search_engine_provider_.GetValue();
}

void SearchEngineProviderService::ChangeToAlternativeSearchEngineProvider() {
  template_url_service_->SetUserSelectedDefaultSearchProvider(
      alternative_search_engine_url_.get());
}

void SearchEngineProviderService::ChangeToNormalWindowSearchEngineProvider() {
  template_url_service_->SetUserSelectedDefaultSearchProvider(nullptr);
}
