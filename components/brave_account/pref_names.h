/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_PREF_NAMES_H_

#include "base/component_export.h"

class PrefRegistrySimple;

namespace brave_account::prefs {

inline constexpr char kTest[] = "brave.account.test";

COMPONENT_EXPORT(BRAVE_ACCOUNT_COMMON)
void RegisterProfilePrefs(PrefRegistrySimple* registry);

}  // namespace brave_account::prefs

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_PREF_NAMES_H_
