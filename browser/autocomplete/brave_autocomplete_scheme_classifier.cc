/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/autocomplete/brave_autocomplete_scheme_classifier.h"

#include "base/strings/string_util.h"
#include "brave/common/url_constants.h"
#include "chrome/browser/profiles/profile.h"

#if !defined(OS_ANDROID)
#include "brave/components/brave_webtorrent/browser/webtorrent_util.h"
#endif

// See the BraveAutocompleteProviderClient why GetOriginalProfile() is fetched.
// All services except TemplateURLService exposed from AutocompleteClassifier
// uses original profile. So, |profile_| should be original profile same as
// base class does.
BraveAutocompleteSchemeClassifier::BraveAutocompleteSchemeClassifier(
    Profile* profile)
    : ChromeAutocompleteSchemeClassifier(profile->GetOriginalProfile()),
      profile_(profile->GetOriginalProfile()) {
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
      (base::LowerCaseEqualsASCII(scheme, kBraveUIScheme)
#if !defined(OS_ANDROID)
      || (webtorrent::IsWebtorrentEnabled(profile_) &&
        base::LowerCaseEqualsASCII(scheme, kMagnetScheme))
#endif
      )) {
    return metrics::OmniboxInputType::URL;
  }

  return ChromeAutocompleteSchemeClassifier::GetInputTypeForScheme(scheme);
}
