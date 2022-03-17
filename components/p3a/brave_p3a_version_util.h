/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_BRAVE_P3A_VERSION_UTIL_H_
#define BRAVE_COMPONENTS_P3A_BRAVE_P3A_VERSION_UTIL_H_

class PrefRegistrySimple;
class PrefService;

namespace brave {

void RegisterP3AVersionUtilPrefs(PrefRegistrySimple* registry);

bool IsBrowserAtLatestVersion(PrefService* local_state);

}  // namespace brave

#endif  // BRAVE_COMPONENTS_P3A_BRAVE_P3A_VERSION_UTIL_H_
