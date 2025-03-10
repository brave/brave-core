/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_EMAIL_ALIASES_BROWSER_EMAIL_ALIASES_PREFS_H_
#define BRAVE_COMPONENTS_EMAIL_ALIASES_BROWSER_EMAIL_ALIASES_PREFS_H_

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace email_aliases {

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

}  // namespace email_aliases

#endif  // BRAVE_COMPONENTS_EMAIL_ALIASES_BROWSER_EMAIL_ALIASES_PREFS_H_
