// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_WEBCOMPAT_REPORTER_BROWSER_WEBCOMPAT_REPORTER_UTILS_H_
#define BRAVE_COMPONENTS_WEBCOMPAT_REPORTER_BROWSER_WEBCOMPAT_REPORTER_UTILS_H_

#include <string>

class PrefRegistrySimple;

namespace webcompat_reporter {

inline constexpr char kContactInfoSaveFlagPrefs[] =
    "brave.webcompat.report.enable_save_contact_info";
inline constexpr char kContactInfoPrefs[] =
    "brave.webcompat.report.contact_info";

bool NeedsToGetComponentInfo(std::string_view component_id);

void RegisterLocalStatePrefs(PrefRegistrySimple* registry);
void RegisterProfilePrefs(PrefRegistrySimple* registry);

std::string BoolToString(bool value);

}  // namespace webcompat_reporter

#endif  // BRAVE_COMPONENTS_WEBCOMPAT_REPORTER_BROWSER_WEBCOMPAT_REPORTER_UTILS_H_
