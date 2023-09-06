/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/omnibox/browser/omnibox_controller.h"

#include "brave/components/omnibox/browser/brave_omnibox_client.h"

#define StartAutocomplete StartAutocomplete_ChromiumImpl
#include "src/components/omnibox/browser/omnibox_controller.cc"
#undef StartAutocomplete

void OmniboxController::StartAutocomplete(
    const AutocompleteInput& input) const {
  auto* client = static_cast<BraveOmniboxClient*>(client_.get());
  if (!client->IsAutocompleteEnabled()) {
    ClearPopupKeywordMode();
    return;
  }

  StartAutocomplete_ChromiumImpl(input);
}
