// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_AUTOCOMPLETE_PROVIDER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_AUTOCOMPLETE_PROVIDER_H_

// Note: We go negative with the BraveAutoCompleteTypes, so we don't conflict if
// Chromium adds something new.
#define TYPE_BOOKMARK \
  TYPE_BRAVE_COMMANDER = -1 << 0, TYPE_BRAVE_LEO = -1 << 1, TYPE_BOOKMARK
#include "src/components/omnibox/browser/autocomplete_provider.h"  // IWYU pragma: export
#undef TYPE_BOOKMARK

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_AUTOCOMPLETE_PROVIDER_H_
