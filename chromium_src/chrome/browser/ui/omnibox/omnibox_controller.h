/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_CONTROLLER_H_

// Also has StartAutocomplete member
#include "chrome/browser/ui/omnibox/omnibox_edit_model.h"

#define StartAutocomplete                                               \
  StartAutocomplete_ChromiumImpl(const AutocompleteInput& input) const; \
  void StartAutocomplete

// Per security/privacy team, we want to disable zero suggest prefetch.
#define StartZeroSuggestPrefetch     \
  StartZeroSuggestPrefetch_Unused(); \
  void StartZeroSuggestPrefetch

#include <chrome/browser/ui/omnibox/omnibox_controller.h>  // IWYU pragma: export
#undef StartZeroSuggestPrefetch
#undef StartAutocomplete

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_CONTROLLER_H_
