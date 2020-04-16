/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/autocomplete/brave_autocomplete_provider_client_for_classifier.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"

BraveAutocompleteProviderClientForClassifier::
BraveAutocompleteProviderClientForClassifier(
    Profile* profile)
    : BraveAutocompleteProviderClient(profile->GetOriginalProfile()),
      profile_(profile) {
}

BraveAutocompleteProviderClientForClassifier::
~BraveAutocompleteProviderClientForClassifier() {
}

TemplateURLService*
    BraveAutocompleteProviderClientForClassifier::GetTemplateURLService() {
  return TemplateURLServiceFactory::GetForProfile(profile_);
}

const TemplateURLService*
BraveAutocompleteProviderClientForClassifier::GetTemplateURLService() const {
  return TemplateURLServiceFactory::GetForProfile(profile_);
}
