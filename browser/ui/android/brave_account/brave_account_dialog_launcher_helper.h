/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_ANDROID_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DIALOG_LAUNCHER_HELPER_H_
#define BRAVE_BROWSER_UI_ANDROID_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DIALOG_LAUNCHER_HELPER_H_

#include <string>

namespace content {
class WebContents;
}

namespace brave_account {

// Opens `url` in its own Brave Account custom tab, over the one serving the
// account rows.
void ShowBraveAccountDialog(content::WebContents* web_contents,
                            const std::string& url);

}  // namespace brave_account

#endif  // BRAVE_BROWSER_UI_ANDROID_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DIALOG_LAUNCHER_HELPER_H_
