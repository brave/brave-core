/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_TRANSLATE_BRAVE_TRANSLATE_PREFS_MIGRATION_H_
#define BRAVE_BROWSER_TRANSLATE_BRAVE_TRANSLATE_PREFS_MIGRATION_H_

class PrefRegistrySimple;
class PrefService;

namespace translate {

namespace prefs {
extern const char kMigratedToInternalTranslation[];
}  // namespace prefs

void RegisterBraveProfilePrefsForMigration(PrefRegistrySimple* registry);

void ClearMigrationBraveProfilePrefs(PrefService* prefs);

}  // namespace translate

#endif  // BRAVE_BROWSER_TRANSLATE_BRAVE_TRANSLATE_PREFS_MIGRATION_H_
