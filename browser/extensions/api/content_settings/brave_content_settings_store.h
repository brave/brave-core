/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_BREVE_CONTENT_SETTINGS_CONTENT_SETTINGS_STORE_H_
#define BRAVE_BROWSER_EXTENSIONS_API_BREVE_CONTENT_SETTINGS_CONTENT_SETTINGS_STORE_H_

#include "chrome/browser/extensions/api/content_settings/content_settings_store.h"

namespace extensions {

// This class is the backend for extension-defined content settings. It is used
// by the content_settings::CustomExtensionProvider to integrate its settings
// into the HostContentSettingsMap and by the content settings extension API to
// provide extensions with access to content settings.
class BraveContentSettingsStore : public ContentSettingsStore {
  public:
    BraveContentSettingsStore();

    // Deserializes content settings rules from |list| and applies them as set by
    // the extension with ID |extension_id|.
    void SetExtensionContentSettingFromList(const std::string& extension_id,
                                            const base::ListValue* list,
                                            ExtensionPrefsScope scope) override;

    void NotifyOfContentSettingChanged(const std::string& extension_id,
                                       bool incognito) override;
  private:
    friend class base::RefCountedThreadSafe<BraveContentSettingsStore>;
    ~BraveContentSettingsStore() override;

    bool suppress_notifications_;

  DISALLOW_COPY_AND_ASSIGN(BraveContentSettingsStore);
};

}  // namespace extensions

#endif // BRAVE_BROWSER_EXTENSIONS_API_BREVE_CONTENT_SETTINGS_CONTENT_SETTINGS_STORE_H_
