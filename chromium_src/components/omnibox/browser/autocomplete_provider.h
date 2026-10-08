// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_AUTOCOMPLETE_PROVIDER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_AUTOCOMPLETE_PROVIDER_H_

// Note: We go negative with the BraveAutoCompleteTypes, so we don't conflict if
// Chromium adds something new.
#define kBookmark kBraveCommander = -1 << 0, kBraveLeo = -1 << 1, kBookmark
#include <components/omnibox/browser/autocomplete_provider.h>  // IWYU pragma: export
#undef kBookmark

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_AUTOCOMPLETE_PROVIDER_H_
