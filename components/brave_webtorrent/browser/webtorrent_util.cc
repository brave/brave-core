/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_webtorrent/browser/webtorrent_util.h"

#include "brave/common/extensions/extension_constants.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "extensions/browser/extension_registry.h"

namespace webtorrent {

bool IsWebtorrentEnabled(content::BrowserContext* browser_context) {
  extensions::ExtensionRegistry* registry =
    extensions::ExtensionRegistry::Get(browser_context);
  return registry->enabled_extensions().Contains(brave_webtorrent_extension_id);
}

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(kWebTorrentEnabled, true);
}

}  // namespace webtorrent

