/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/search_engines/private_window_search_engine_provider_service.h"

#include "chrome/browser/profiles/profile.h"
#include "components/search_engines/template_url_service.h"

PrivateWindowSearchEngineProviderService::
PrivateWindowSearchEngineProviderService(Profile* otr_profile)
    : SearchEngineProviderService(otr_profile) {
  DCHECK(otr_profile->IsIncognitoProfile());

  // Monitor normal profile's search engine changing because private window
  // should that search engine provider when alternative search engine isn't
  // used.
  original_template_url_service_->AddObserver(this);
  ConfigureSearchEngineProvider();
}

PrivateWindowSearchEngineProviderService::
~PrivateWindowSearchEngineProviderService() {
  original_template_url_service_->RemoveObserver(this);
}

void PrivateWindowSearchEngineProviderService::
OnUseAlternativeSearchEngineProviderChanged() {
  ConfigureSearchEngineProvider();
}

void PrivateWindowSearchEngineProviderService::
ConfigureSearchEngineProvider() {
  UseAlternativeSearchEngineProvider()
      ? ChangeToAlternativeSearchEngineProvider()
      : ChangeToNormalWindowSearchEngineProvider();
}

void
PrivateWindowSearchEngineProviderService::OnTemplateURLServiceChanged() {
  // If private window uses alternative, search provider changing of normal
  // profile should not affect private window's provider.
  if (UseAlternativeSearchEngineProvider())
    return;

  // When normal profile's default search provider is changed, apply it to
  // private window's provider.
  ChangeToNormalWindowSearchEngineProvider();
}

