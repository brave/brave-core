/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_ORIGIN_ANDROID_BRAVE_ORIGIN_SETTINGS_LAUNCHER_HELPER_H_
#define BRAVE_BROWSER_UI_BRAVE_ORIGIN_ANDROID_BRAVE_ORIGIN_SETTINGS_LAUNCHER_HELPER_H_

class Profile;

namespace brave_origin {

// Opens the Brave Origin settings screen and asks it to surface the restart
// prompt. Called when a purchase is first detected so the user can restart to
// apply the newly enforced policies.
void OpenOriginSettingsForRestart(Profile* profile);

}  // namespace brave_origin

#endif  // BRAVE_BROWSER_UI_BRAVE_ORIGIN_ANDROID_BRAVE_ORIGIN_SETTINGS_LAUNCHER_HELPER_H_
