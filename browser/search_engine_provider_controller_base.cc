/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/search_engine_provider_controller_base.h"

#include "brave/common/pref_names.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/prepopulated_engines.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "components/search_engines/template_url_service.h"

SearchEngineProviderControllerBase::SearchEngineProviderControllerBase(
    Profile* profile)
    : otr_profile_(profile),
      original_template_url_service_(
          TemplateURLServiceFactory::GetForProfile(
              otr_profile_->GetOriginalProfile())),
      otr_template_url_service_(
          TemplateURLServiceFactory::GetForProfile(otr_profile_)) {
  use_alternative_search_engine_provider_.Init(
      kUseAlternativeSearchEngineProvider,
      otr_profile_->GetOriginalProfile()->GetPrefs(),
      base::Bind(&SearchEngineProviderControllerBase::OnPreferenceChanged,
                 base::Unretained(this)));

  auto data = TemplateURLPrepopulateData::GetPrepopulatedEngine(
      profile->GetPrefs(),
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO);
  alternative_search_engine_url_.reset(new TemplateURL(*data));
}

SearchEngineProviderControllerBase::~SearchEngineProviderControllerBase() {
}

void SearchEngineProviderControllerBase::OnTemplateURLServiceShuttingDown() {
  delete this;
}

void SearchEngineProviderControllerBase::OnPreferenceChanged(
    const std::string& pref_name) {
  DCHECK(pref_name == kUseAlternativeSearchEngineProvider);

  ConfigureSearchEngineProvider();
}

bool
SearchEngineProviderControllerBase::UseAlternativeSearchEngineProvider() const {
  // Currently, use alternative search engine provider for tor profile.
  // TODO(simonhong): Revisit when related setting ux is determined.
  if (otr_profile_->IsTorProfile())
    return true;

  return use_alternative_search_engine_provider_.GetValue();
}

void SearchEngineProviderControllerBase::
ChangeToAlternativeSearchEngineProvider() {
  otr_template_url_service_->SetUserSelectedDefaultSearchProvider(
      alternative_search_engine_url_.get());
}

void SearchEngineProviderControllerBase::
ChangeToNormalWindowSearchEngineProvider() {
  TemplateURL normal_url(
      original_template_url_service_->GetDefaultSearchProvider()->data());
  otr_template_url_service_->SetUserSelectedDefaultSearchProvider(
      &normal_url);
}

