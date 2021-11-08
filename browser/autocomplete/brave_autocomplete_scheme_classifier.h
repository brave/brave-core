/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_AUTOCOMPLETE_BRAVE_AUTOCOMPLETE_SCHEME_CLASSIFIER_H_
#define BRAVE_BROWSER_AUTOCOMPLETE_BRAVE_AUTOCOMPLETE_SCHEME_CLASSIFIER_H_

#include <string>

#include "brave/components/brave_webtorrent/browser/buildflags/buildflags.h"
#include "chrome/browser/autocomplete/chrome_autocomplete_scheme_classifier.h"

class BraveAutocompleteSchemeClassifier
    : public ChromeAutocompleteSchemeClassifier {
 public:
  explicit BraveAutocompleteSchemeClassifier(Profile* profile);
  BraveAutocompleteSchemeClassifier(const BraveAutocompleteSchemeClassifier&) =
      delete;
  BraveAutocompleteSchemeClassifier& operator=(
      const BraveAutocompleteSchemeClassifier&) = delete;
  ~BraveAutocompleteSchemeClassifier() override;

  metrics::OmniboxInputType GetInputTypeForScheme(
      const std::string& scheme) const override;

 private:
#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
  Profile* profile_ = nullptr;
#endif
};

#endif  // BRAVE_BROWSER_AUTOCOMPLETE_BRAVE_AUTOCOMPLETE_SCHEME_CLASSIFIER_H_

