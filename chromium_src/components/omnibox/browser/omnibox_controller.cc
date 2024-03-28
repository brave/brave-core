/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/omnibox/browser/omnibox_controller.h"

#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"
#include "components/prefs/pref_service.h"

namespace {

bool IsAutocompleteEnabled(const PrefService* prefs) {
  return prefs->GetBoolean(omnibox::kAutocompleteEnabled);
}

}  // namespace

#define StartAutocomplete StartAutocomplete_ChromiumImpl
#include "src/components/omnibox/browser/omnibox_controller.cc"
#undef StartAutocomplete

void OmniboxController::StartAutocomplete(
    const AutocompleteInput& input) const {
  if (!IsAutocompleteEnabled(client_->GetPrefs())) {
    ClearPopupKeywordMode();
    return;
  }

  StartAutocomplete_ChromiumImpl(input);
}
