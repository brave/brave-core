/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SKUS_BROWSER_PREF_NAMES_H_
#define BRAVE_COMPONENTS_SKUS_BROWSER_PREF_NAMES_H_

namespace skus {
namespace prefs {

// Dictionary storage for the SKU SDK. For example, account.brave.com
// stores SKU key/value pairs in local storage.
inline constexpr char kSkusState[] = "skus.state";
inline constexpr char kSkusStateMigratedToLocalState[] =
    "skus.state.migrated_to_local_state";

}  // namespace prefs
}  // namespace skus

#endif  // BRAVE_COMPONENTS_SKUS_BROWSER_PREF_NAMES_H_
