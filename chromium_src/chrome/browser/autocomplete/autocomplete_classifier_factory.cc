/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/autocomplete/brave_autocomplete_provider_client_for_classifier.h"
#include "brave/browser/autocomplete/brave_autocomplete_scheme_classifier.h"
#include "components/omnibox/browser/autocomplete_classifier.h"
#include "components/omnibox/browser/autocomplete_controller.h"

#define ChromeAutocompleteProviderClient BraveAutocompleteProviderClientForClassifier  // NOLINT
#define ChromeAutocompleteSchemeClassifier BraveAutocompleteSchemeClassifier
#include "../../../../../chrome/browser/autocomplete/autocomplete_classifier_factory.cc"
#undef ChromeAutocompleteSchemeClassifier
#undef ChromeAutocompleteProviderClient
