/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/content_settings/brave_content_settings_store.h"

#define ContentSettingsStore BraveContentSettingsStore
#include "../../../../../../../chrome/browser/extensions/api/content_settings/content_settings_service.cc"
#undef ContentSettingsStore
