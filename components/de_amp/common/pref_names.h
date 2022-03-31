/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_DE_AMP_COMMON_PREF_NAMES_H_
#define BRAVE_COMPONENTS_DE_AMP_COMMON_PREF_NAMES_H_

class PrefRegistrySimple;

namespace de_amp {

// Is De-AMP feature currently enabled
extern const char kDeAmpPrefEnabled[];

void RegisterProfilePrefs(PrefRegistrySimple* registry);

}  // namespace de_amp

#endif  // BRAVE_COMPONENTS_DE_AMP_COMMON_PREF_NAMES_H_
