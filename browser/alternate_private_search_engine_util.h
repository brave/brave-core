/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_ALTERNATE_PRIVATE_SEARCH_ENGINE_UTIL_H_
#define BRAVE_BROWSER_ALTERNATE_PRIVATE_SEARCH_ENGINE_UTIL_H_

class Profile;

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace brave {

bool UseAlternatePrivateSearchEngineEnabled(Profile* profile);

void RegisterAlternatePrivateSearchEngineProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry);

void ToggleUseAlternatePrivateSearchEngine(Profile* profile);

}  // namespace brave

#endif  // BRAVE_BROWSER_ALTERNATE_PRIVATE_SEARCH_ENGINE_UTIL_H_
