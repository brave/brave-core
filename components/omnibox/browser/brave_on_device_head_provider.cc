// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/omnibox/browser/brave_on_device_head_provider.h"

#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"
#include "components/omnibox/browser/autocomplete_provider_client.h"
#include "components/omnibox/browser/autocomplete_provider_listener.h"
#include "components/prefs/pref_service.h"

// static
BraveOnDeviceHeadProvider* BraveOnDeviceHeadProvider::Create(
    AutocompleteProviderClient* client,
    AutocompleteProviderListener* listener) {
  return new BraveOnDeviceHeadProvider(client, listener);
}

void BraveOnDeviceHeadProvider::Start(const AutocompleteInput& input,
                                      bool minimal_changes) {
  auto* prefs = client_->GetPrefs();
  if (!prefs || !prefs->GetBoolean(omnibox::kOnDeviceSuggestionsEnabled)) {
    matches_.clear();
    return;
  }
  OnDeviceHeadProvider::Start(input, minimal_changes);
}

BraveOnDeviceHeadProvider::~BraveOnDeviceHeadProvider() = default;
