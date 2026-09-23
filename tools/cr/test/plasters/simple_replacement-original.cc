// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/autocomplete/autocomplete_classifier_factory.h"

// static
std::unique_ptr<KeyedService> AutocompleteClassifierFactory::BuildInstanceFor(
    content::BrowserContext* context) {
  Profile* profile = static_cast<Profile*>(context);
  return std::make_unique<AutocompleteClassifier>(
      std::make_unique<AutocompleteController>(
          std::make_unique<ChromeAutocompleteProviderClient>(profile),
          AutocompleteClassifier::DefaultOmniboxProviders()),
      std::make_unique<ChromeAutocompleteSchemeClassifier>(profile));
}
