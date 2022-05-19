/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_SETTINGS_LOCALIZED_STRINGS_PROVIDER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_SETTINGS_LOCALIZED_STRINGS_PROVIDER_H_

class Profile;

namespace content {
class WebUIDataSource;
}

namespace settings {

void BraveAddLocalizedStrings(content::WebUIDataSource*, Profile*);

}  // namespace settings

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_SETTINGS_LOCALIZED_STRINGS_PROVIDER_H_
