/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_EXTENSIONS_BROWSER_BREVE_EXTENSION_PREFS_H_
#define BRAVE_EXTENSIONS_BROWSER_BREVE_EXTENSION_PREFS_H_

#include "extensions/browser/extension_prefs.h"

namespace extensions {

class BraveExtensionPrefs : public ExtensionPrefs {
  public:
    BraveExtensionPrefs(
        content::BrowserContext* browser_context,
        PrefService* prefs,
        const base::FilePath& root_dir,
        ExtensionPrefValueMap* extension_pref_value_map,
        std::unique_ptr<base::Clock> clock,
        bool extensions_disabled,
        const std::vector<ExtensionPrefsObserver*>& early_observers);

    ~BraveExtensionPrefs() override;

    void NotifyExtensionPrefsLoaded(const std::string& extension_id);

  DISALLOW_COPY_AND_ASSIGN(BraveExtensionPrefs);
};

}  // namespace extensions

#endif // BRAVE_EXTENSIONS_BROWSER_BREVE_EXTENSION_PREFS_H_
