/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/brave_history_quick_provider.h"

#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"
#include "components/omnibox/browser/actions/omnibox_action.h"
#include "components/omnibox/browser/autocomplete_provider.h"
#include "components/omnibox/browser/history_quick_provider.h"
#include "components/prefs/pref_service.h"

BraveHistoryQuickProvider::~BraveHistoryQuickProvider() = default;

void BraveHistoryQuickProvider::Start(const AutocompleteInput& input,
                                      bool minimal_changes) {
  if (!client()->GetPrefs()->GetBoolean(omnibox::kHistorySuggestionsEnabled)) {
    matches_.clear();
    return;
  }
  HistoryQuickProvider::Start(input, minimal_changes);
}
