/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/omnibox/browser/omnibox_controller.h"

#define StartAutocomplete StartAutocomplete_ChromiumImpl
#include "src/components/omnibox/browser/omnibox_controller.cc"
#undef StartAutocomplete

void OmniboxController::StartAutocomplete(
    const AutocompleteInput& input) const {
  if (!client_->IsAutocompleteEnabled()) {
    ClearPopupKeywordMode();
    return;
  }

  StartAutocomplete_ChromiumImpl(input);
}
