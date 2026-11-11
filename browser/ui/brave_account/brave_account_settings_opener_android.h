/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SETTINGS_OPENER_ANDROID_H_
#define BRAVE_BROWSER_UI_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SETTINGS_OPENER_ANDROID_H_

#include "brave/components/brave_account/mojom/brave_account.mojom-forward.h"

namespace content {
class WebContents;
}

namespace brave_account {

// Opens the native settings screen for `page` and, if it opened, closes
// `web_contents`, the page that linked to it. Does nothing if `web_contents`
// has no window or activity, if `page` isn't handled, or if `web_contents`
// already opened a screen.
void OpenSettingsPage(content::WebContents& web_contents,
                      mojom::SettingsPage page);

}  // namespace brave_account

#endif  // BRAVE_BROWSER_UI_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SETTINGS_OPENER_ANDROID_H_
