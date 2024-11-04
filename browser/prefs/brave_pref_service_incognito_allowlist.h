/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PREFS_BRAVE_PREF_SERVICE_INCOGNITO_ALLOWLIST_H_
#define BRAVE_BROWSER_PREFS_BRAVE_PREF_SERVICE_INCOGNITO_ALLOWLIST_H_

#include "base/containers/span.h"
#include "base/strings/cstring_view.h"

namespace brave {

// Returns names of preferences that should be persistent on incognito profile.
// This list should be preferred over GetOriginalProfile().
base::span<const base::cstring_view> GetBravePersistentPrefNames();

}  // namespace brave

#endif  // BRAVE_BROWSER_PREFS_BRAVE_PREF_SERVICE_INCOGNITO_ALLOWLIST_H_
