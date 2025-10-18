/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_PREF_NAMES_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_PREF_NAMES_H_

#include <components/content_settings/core/common/pref_names.h>  // IWYU pragma: export

namespace prefs {

// Preferences that are exclusively used to store managed values for default
// content settings.
inline constexpr char kManagedDefaultBraveHttpsUpgrade[] =
    "brave.profile.managed_default_content_settings.brave_https_upgrade";
inline constexpr char kManagedDefaultBraveReferrers[] =
    "brave.profile.managed_default_content_settings.brave_referrers";

}  // namespace prefs

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_PREF_NAMES_H_
