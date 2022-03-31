/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/fake_autocomplete_provider_client.h"

#include "brave/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"

FakeAutocompleteProviderClient::FakeAutocompleteProviderClient() {
  pref_service_ = std::make_unique<TestingPrefServiceSimple>();
  auto* registry = pref_service_->registry();
  registry->RegisterBooleanPref(kTopSiteSuggestionsEnabled, true);
  registry->RegisterBooleanPref(kBraveSuggestedSiteSuggestionsEnabled, true);
}

FakeAutocompleteProviderClient::~FakeAutocompleteProviderClient() = default;

PrefService* FakeAutocompleteProviderClient::GetPrefs() const {
  return pref_service_.get();
}
