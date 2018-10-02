/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/search_engine_provider_controller_base.h"

#include "base/strings/utf_string_conversions.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/template_url_service.h"

namespace {

TemplateURLData GetAlternativeSearchEngineData() {
  TemplateURLData private_search_engine_data;
  private_search_engine_data.SetShortName(base::ASCIIToUTF16("DuckDuckGo"));
  private_search_engine_data.SetKeyword(base::ASCIIToUTF16("duckduckgo.com"));
  private_search_engine_data.SetURL(
      "https://duckduckgo.com/?q={searchTerms}&t=brave");
  private_search_engine_data.favicon_url =
      GURL("https://duckduckgo.com/favicon.ico");
  private_search_engine_data.suggestions_url =
      "https://duckduckgo.com/ac/?q={searchTerms}&type=list";
  return private_search_engine_data;
}

}  // namespace

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

  alternative_search_engine_url_.reset(
      new TemplateURL(GetAlternativeSearchEngineData()));
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

