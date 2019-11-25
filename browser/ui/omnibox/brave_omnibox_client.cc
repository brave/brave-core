/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/omnibox/brave_omnibox_client.h"

#include "brave/browser/autocomplete/brave_autocomplete_scheme_classifier.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/omnibox/chrome_omnibox_client.h"
#include "chrome/browser/ui/omnibox/chrome_omnibox_edit_controller.h"
#include "components/prefs/pref_service.h"

BraveOmniboxClient::BraveOmniboxClient(OmniboxEditController* controller,
                                                        Profile* profile)
      : ChromeOmniboxClient(controller, profile),
        profile_(profile),
        scheme_classifier_(profile) {}

BraveOmniboxClient::~BraveOmniboxClient() {}

const AutocompleteSchemeClassifier&
                          BraveOmniboxClient::GetSchemeClassifier() const {
  return scheme_classifier_;
}

bool BraveOmniboxClient::IsAutocompleteEnabled() const {
  return profile_->GetPrefs()->GetBoolean(kAutocompleteEnabled);
}
