/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/alternate_private_search_engine_controller.h"

#include "base/strings/utf_string_conversions.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/template_url_service.h"

namespace {

TemplateURLData GetPrivateSearchEngineData() {
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

// static
void AlternatePrivateSearchEngineController::Create(Profile* profile) {
  // This controller is deleted by itself when observed TemplateURLSearvice is
  // destroyed.
  new AlternatePrivateSearchEngineController(profile);
}

AlternatePrivateSearchEngineController::AlternatePrivateSearchEngineController(
    Profile* profile)
    : profile_(profile),
      original_template_url_service_(
          TemplateURLServiceFactory::GetForProfile(
              profile_->GetOriginalProfile())),
      private_template_url_service_(
          TemplateURLServiceFactory::GetForProfile(profile_)) {
  DCHECK_EQ(profile_->GetProfileType(), Profile::INCOGNITO_PROFILE);

  use_alternate_private_search_engine_enabled_.Init(
      kUseAlternatePrivateSearchEngine,
      profile_->GetOriginalProfile()->GetPrefs(),
      base::Bind(&AlternatePrivateSearchEngineController::OnPreferenceChanged,
                 base::Unretained(this)));

  original_template_url_service_->AddObserver(this);
  private_search_engine_url_.reset(
      new TemplateURL(GetPrivateSearchEngineData()));
  ConfigureAlternatePrivateSearchEngineProvider();
}

AlternatePrivateSearchEngineController::
~AlternatePrivateSearchEngineController() {
}

void AlternatePrivateSearchEngineController::OnTemplateURLServiceChanged() {
  // If private mode uses ddg, search provider changing of original profile can
  // be ignored.
  if (use_alternate_private_search_engine_enabled_.GetValue())
    return;

  // When normal profile's default search provider is changed, apply it to
  // alternate search engine again if private mode doesn't use ddg.
  SetNormalModeDefaultSearchEngineAsDefaultPrivateSearchProvider();
}

void
AlternatePrivateSearchEngineController::OnTemplateURLServiceShuttingDown() {
  original_template_url_service_->RemoveObserver(this);
  delete this;
}

void AlternatePrivateSearchEngineController::
SetAlternateDefaultPrivateSearchEngine() {
  private_template_url_service_->SetUserSelectedDefaultSearchProvider(
      private_search_engine_url_.get());
}

void AlternatePrivateSearchEngineController::
SetNormalModeDefaultSearchEngineAsDefaultPrivateSearchProvider() {
  TemplateURL normal_url(
      original_template_url_service_->GetDefaultSearchProvider()->data());
  private_template_url_service_->SetUserSelectedDefaultSearchProvider(
      &normal_url);
}

void AlternatePrivateSearchEngineController::
ConfigureAlternatePrivateSearchEngineProvider() {
  use_alternate_private_search_engine_enabled_.GetValue()
      ? SetAlternateDefaultPrivateSearchEngine()
      : SetNormalModeDefaultSearchEngineAsDefaultPrivateSearchProvider();
}

void AlternatePrivateSearchEngineController::OnPreferenceChanged(
    const std::string& pref_name) {
  DCHECK(pref_name == kUseAlternatePrivateSearchEngine);

  ConfigureAlternatePrivateSearchEngineProvider();
}
