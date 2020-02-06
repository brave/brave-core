/* This Source Code Form is subject to the terms of the Mozilla Public                               
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,                         
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/autocomplete/brave_autocomplete_provider_client.h"                           
#include "brave/browser/autocomplete/brave_autocomplete_scheme_classifier.h"                         

#define ChromeAutocompleteSchemeClassifier BraveAutocompleteSchemeClassifier                         
#define ChromeAutocompleteProviderClient BraveAutocompleteProviderClient                             
#include "../../../../../../chrome/browser/ui/search/search_tab_helper.cc"
#undef ChromeAutocompleteProviderClient
#undef ChromeAutocompleteSchemeClassifier

