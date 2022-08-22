/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/autocomplete/brave_autocomplete_scheme_classifier.h"

#include <string>

#include "base/strings/string_util.h"
#include "brave/components/constants/url_constants.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"

#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
#include "brave/components/brave_webtorrent/browser/webtorrent_util.h"
#endif

#if BUILDFLAG(ENABLE_IPFS)
#include "brave/components/ipfs/ipfs_constants.h"
#endif

BraveAutocompleteSchemeClassifier::BraveAutocompleteSchemeClassifier(
    Profile* profile)
    : ChromeAutocompleteSchemeClassifier(profile) {
#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
  profile_ = profile;
#endif
}

BraveAutocompleteSchemeClassifier::~BraveAutocompleteSchemeClassifier() =
    default;

// Without this override, typing in brave:// URLs will search Google
metrics::OmniboxInputType
BraveAutocompleteSchemeClassifier::GetInputTypeForScheme(
    const std::string& scheme) const {
  if (scheme.empty()) {
    return metrics::OmniboxInputType::EMPTY;
  }
  if (base::IsStringASCII(scheme) &&
      base::EqualsCaseInsensitiveASCII(scheme, kBraveUIScheme)) {
    return metrics::OmniboxInputType::URL;
  }

#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
  if (base::IsStringASCII(scheme) &&
      webtorrent::IsWebtorrentPrefEnabled(profile_) &&
      base::EqualsCaseInsensitiveASCII(scheme, kMagnetScheme)) {
    return metrics::OmniboxInputType::URL;
  }
#endif

#if BUILDFLAG(ENABLE_IPFS)
  if (base::IsStringASCII(scheme) &&
      (base::EqualsCaseInsensitiveASCII(scheme, ipfs::kIPFSScheme) ||
       base::EqualsCaseInsensitiveASCII(scheme, ipfs::kIPNSScheme))) {
    return metrics::OmniboxInputType::URL;
  }
#endif


  return ChromeAutocompleteSchemeClassifier::GetInputTypeForScheme(scheme);
}
