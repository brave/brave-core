/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/content_settings/brave_content_settings_store.h"

namespace extensions {

BraveContentSettingsStore::BraveContentSettingsStore()
  : suppress_notifications_(false) {
}

BraveContentSettingsStore::~BraveContentSettingsStore() {
}

void BraveContentSettingsStore::SetExtensionContentSettingFromList(
    const std::string& extension_id,
    const base::ListValue* list,
    ExtensionPrefsScope scope) {
  if (list->GetList().empty()) return;

  // For SetExtensionContentSettingFromList use case, we delay the notification
  // after the values in the list from extension_prefs are all stored in the
  // content setting store. The delay of notification is needed because the
  // values saved in extension_prefs will be overwritten by the one saved in
  // content setting store in PreferenceAPI::OnContentSettingChanged. Without
  // the delay, only the first entry in the list could be saved into content
  // setting store.
  suppress_notifications_ = true;
  ContentSettingsStore::SetExtensionContentSettingFromList(
      extension_id, list, scope);
  suppress_notifications_ = false;

  // Send a single notification for the entire list.
  NotifyOfContentSettingChanged(extension_id,
                                scope != kExtensionPrefsScopeRegular);
}

void BraveContentSettingsStore::NotifyOfContentSettingChanged(
    const std::string& extension_id,
    bool incognito) {
  if (suppress_notifications_) return;
  ContentSettingsStore::NotifyOfContentSettingChanged(extension_id, incognito);
}

} // namespace extensions
