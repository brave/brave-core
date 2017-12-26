/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "brave/components/omnibox/browser/brave_autocomplete_controller.h"
#include "brave/components/omnibox/browser/topsites_provider.h"



BraveAutocompleteController::BraveAutocompleteController(
    std::unique_ptr<AutocompleteProviderClient> provider_client,
    AutocompleteControllerDelegate* delegate,
    int provider_types) :
    AutocompleteController(
        std::move(provider_client),
        delegate,
        provider_types)
 {
    if (provider_types & AutocompleteProvider::TYPE_SEARCH) {
      providers_.push_back(new TopSitesProvider(provider_client_.get()));
    }
 }

 BraveAutocompleteController::~BraveAutocompleteController() {

 }
