/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/private_window_search_engine_provider_controller.h"

#include "chrome/browser/profiles/profile.h"
#include "components/search_engines/template_url_service.h"

PrivateWindowSearchEngineProviderController::
PrivateWindowSearchEngineProviderController(Profile* profile)
    : SearchEngineProviderControllerBase(profile) {
  DCHECK_EQ(profile->GetProfileType(), Profile::INCOGNITO_PROFILE);

  // Monitor normal profile's search engine changing because private window
  // should that search engine provider when alternative search engine isn't
  // used.
  original_template_url_service_->AddObserver(this);
  ConfigureSearchEngineProvider();
}

PrivateWindowSearchEngineProviderController::
~PrivateWindowSearchEngineProviderController() {
  original_template_url_service_->RemoveObserver(this);
}

void PrivateWindowSearchEngineProviderController::
ConfigureSearchEngineProvider() {
  UseAlternativeSearchEngineProvider()
      ? ChangeToAlternativeSearchEngineProvider()
      : ChangeToNormalWindowSearchEngineProvider();
}

void
PrivateWindowSearchEngineProviderController::OnTemplateURLServiceChanged() {
  // If private window uses alternative, search provider changing of normal
  // profile should not affect private window's provider.
  if (UseAlternativeSearchEngineProvider())
    return;

  // When normal profile's default search provider is changed, apply it to
  // private window's provider.
  ChangeToNormalWindowSearchEngineProvider();
}

void PrivateWindowSearchEngineProviderController::
ChangeToAlternativeSearchEngineProvider() {
  otr_template_url_service_->SetUserSelectedDefaultSearchProvider(
      alternative_search_engine_url_.get());
}

void PrivateWindowSearchEngineProviderController::
ChangeToNormalWindowSearchEngineProvider() {
  TemplateURL normal_url(
      original_template_url_service_->GetDefaultSearchProvider()->data());
  otr_template_url_service_->SetUserSelectedDefaultSearchProvider(
      &normal_url);
}

