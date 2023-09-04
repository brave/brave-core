/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_OMNIBOX_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_OMNIBOX_CONTROLLER_H_

#define StartAutocomplete virtual StartAutocomplete
#define SetRichSuggestionBitmap        \
  SetRichSuggestionBitmapUnused();     \
  friend class BraveOmniboxController; \
  void SetRichSuggestionBitmap
#include "src/components/omnibox/browser/omnibox_controller.h"  // IWYU pragma: export
#undef StartAutocomplete
#undef SetRichSuggestionBitmap

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_OMNIBOX_CONTROLLER_H_
