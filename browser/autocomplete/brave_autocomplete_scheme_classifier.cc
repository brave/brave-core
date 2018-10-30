/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/autocomplete/brave_autocomplete_scheme_classifier.h"

#include "base/strings/string_util.h"
#include "brave/common/url_constants.h"
#include "chrome/browser/profiles/profile.h"

// See the BraveAutocompleteProviderClient why GetOriginalProfile() is fetched.
BraveAutocompleteSchemeClassifier::BraveAutocompleteSchemeClassifier(
    Profile* profile)
    : ChromeAutocompleteSchemeClassifier(profile->GetOriginalProfile()) {
}

BraveAutocompleteSchemeClassifier::~BraveAutocompleteSchemeClassifier() {
}

// Without this override, typing in brave:// URLs will search Google
metrics::OmniboxInputType
BraveAutocompleteSchemeClassifier::GetInputTypeForScheme(
    const std::string& scheme) const {
  if (scheme.empty()) {
    return metrics::OmniboxInputType::INVALID;
  }
  if (base::IsStringASCII(scheme) &&
       base::LowerCaseEqualsASCII(scheme, kBraveUIScheme)) {
    return metrics::OmniboxInputType::URL;
  }

  return ChromeAutocompleteSchemeClassifier::GetInputTypeForScheme(scheme);
}
