// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_SERVICE_PREFS_H_
#define BRAVE_IOS_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_SERVICE_PREFS_H_

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace search_engines {

// Registers the prefs backing the private-mode default search engine.
void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

}  // namespace search_engines

#endif  // BRAVE_IOS_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_SERVICE_PREFS_H_
