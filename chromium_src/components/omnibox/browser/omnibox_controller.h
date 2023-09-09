/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_OMNIBOX_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_OMNIBOX_CONTROLLER_H_

// Also has StartAutocomplete member
#include "components/omnibox/browser/omnibox_edit_model.h"

#define StartAutocomplete                                               \
  StartAutocomplete_ChromiumImpl(const AutocompleteInput& input) const; \
  void StartAutocomplete

#include "src/components/omnibox/browser/omnibox_controller.h"  // IWYU pragma: export
#undef StartAutocomplete

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_OMNIBOX_CONTROLLER_H_
