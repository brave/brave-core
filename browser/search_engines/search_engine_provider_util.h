/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SEARCH_ENGINES_SEARCH_ENGINE_PROVIDER_UTIL_H_
#define BRAVE_BROWSER_SEARCH_ENGINES_SEARCH_ENGINE_PROVIDER_UTIL_H_

class Profile;

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace brave {

bool UseAlternativeSearchEngineProviderEnabled(Profile* profile);

void ToggleUseAlternativeSearchEngineProvider(Profile* profile);

bool IsRegionForQwant(Profile* profile);

}  // namespace brave

#endif  // BRAVE_BROWSER_SEARCH_ENGINES_SEARCH_ENGINE_PROVIDER_UTIL_H_
