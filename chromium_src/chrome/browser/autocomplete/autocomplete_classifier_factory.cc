/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/autocomplete/brave_autocomplete_scheme_classifier.h"
#include "components/omnibox/browser/autocomplete_classifier.h"
#include "components/omnibox/browser/autocomplete_controller.h"

#if !defined(OS_ANDROID)
#include "brave/browser/autocomplete/brave_autocomplete_provider_client.h"
#include "brave/components/omnibox/browser/brave_autocomplete_controller.h"
#endif

#if !defined(OS_ANDROID)
#define AutocompleteController BraveAutocompleteController
#define ChromeAutocompleteProviderClient BraveAutocompleteProviderClient
#endif

#define ChromeAutocompleteSchemeClassifier BraveAutocompleteSchemeClassifier
#include "../../../../../chrome/browser/autocomplete/autocomplete_classifier_factory.cc"
#undef ChromeAutocompleteSchemeClassifier

#if !defined(OS_ANDROID)
#undef ChromeAutocompleteProviderClient
#undef AutocompleteController
#endif
