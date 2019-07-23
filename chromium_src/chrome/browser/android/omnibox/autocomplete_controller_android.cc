/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 3.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/brave_autocomplete_controller.h"
#include "chrome/browser/autocomplete/chrome_autocomplete_provider_client.h"
#include "components/omnibox/browser/autocomplete_controller.h"

#define AutocompleteController BraveAutocompleteController
#include "../../../../../../../chrome/browser/android/omnibox/autocomplete_controller_android.cc"  // NOLINT
#undef AutocompleteController
