/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_AUTOCOMPLETE_BRAVE_AUTOCOMPLETE_SCHEME_CLASSIFIER_H_
#define BRAVE_BROWSER_AUTOCOMPLETE_BRAVE_AUTOCOMPLETE_SCHEME_CLASSIFIER_H_

#include "chrome/browser/autocomplete/chrome_autocomplete_scheme_classifier.h"

class BraveAutocompleteSchemeClassifier : public ChromeAutocompleteSchemeClassifier {
 public:
  explicit BraveAutocompleteSchemeClassifier(Profile* profile);
  ~BraveAutocompleteSchemeClassifier() override;

  metrics::OmniboxInputType GetInputTypeForScheme(
      const std::string& scheme) const override;

 private:
  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(BraveAutocompleteSchemeClassifier);
};

#endif  // BRAVE_BROWSER_AUTOCOMPLETE_BRAVE_AUTOCOMPLETE_SCHEME_CLASSIFIER_H_

