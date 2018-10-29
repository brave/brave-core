/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/autocomplete/brave_autocomplete_provider_client.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"

BraveAutocompleteProviderClient::BraveAutocompleteProviderClient(
    Profile* profile)
    : ChromeAutocompleteProviderClient(profile->GetOriginalProfile()),
      profile_(profile) {
}

BraveAutocompleteProviderClient::~BraveAutocompleteProviderClient() {
}

TemplateURLService* BraveAutocompleteProviderClient::GetTemplateURLService() {
  return TemplateURLServiceFactory::GetForProfile(profile_);
}

const TemplateURLService*
BraveAutocompleteProviderClient::GetTemplateURLService() const {
  return TemplateURLServiceFactory::GetForProfile(profile_);
}
