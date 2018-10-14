// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/omnibox/brave_omnibox_client.h"

#include "brave/browser/autocomplete/brave_autocomplete_scheme_classifier.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/omnibox/chrome_omnibox_client.h"
#include "chrome/browser/ui/omnibox/chrome_omnibox_edit_controller.h"

BraveOmniboxClient::BraveOmniboxClient(OmniboxEditController* controller,
                                                        Profile* profile)
      : ChromeOmniboxClient(controller, profile),
        scheme_classifier_(profile) {}

BraveOmniboxClient::~BraveOmniboxClient() {}

const AutocompleteSchemeClassifier&
                          BraveOmniboxClient::GetSchemeClassifier() const {
  return scheme_classifier_;
}
