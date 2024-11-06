// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_WEBCOMPAT_REPORTER_COMMON_PREF_NAMES_H_
#define BRAVE_COMPONENTS_WEBCOMPAT_REPORTER_COMMON_PREF_NAMES_H_

class PrefRegistrySimple;
namespace webcompat_reporter {
namespace prefs {

inline constexpr char kContactInfoSaveFlagPrefs[] =
    "brave.webcompat.report.enable_save_contact_info";
inline constexpr char kContactInfoPrefs[] =
    "brave.webcompat.report.contact_info";

void RegisterProfilePrefs(PrefRegistrySimple* registry);

}  // namespace prefs
}  // namespace webcompat_reporter

#endif  // BRAVE_COMPONENTS_WEBCOMPAT_REPORTER_COMMON_PREF_NAMES_H_
