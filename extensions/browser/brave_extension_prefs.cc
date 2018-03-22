/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/time/clock.h"
#include "brave/extensions/browser/brave_extension_prefs.h"
#include "extensions/browser/extension_prefs_observer.h"

namespace extensions {

BraveExtensionPrefs::BraveExtensionPrefs(
    content::BrowserContext* browser_context,
    PrefService* prefs,
    const base::FilePath& root_dir,
    ExtensionPrefValueMap* extension_pref_value_map,
    std::unique_ptr<base::Clock> clock,
    bool extensions_disabled,
    const std::vector<ExtensionPrefsObserver*>& early_observers)
    : ExtensionPrefs(browser_context,
                     prefs,
                     root_dir,
                     extension_pref_value_map,
                     std::move(clock),
                     extensions_disabled,
                     early_observers) {
}

BraveExtensionPrefs::~BraveExtensionPrefs() {
}

void BraveExtensionPrefs::NotifyExtensionPrefsLoaded(
    const std::string& extension_id) {
  for (auto& observer : observer_list_)
    observer.OnExtensionPrefsLoaded(extension_id, this);
}

} // namespace extensions
