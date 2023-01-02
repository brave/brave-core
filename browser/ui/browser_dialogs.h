/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BROWSER_DIALOGS_H_
#define BRAVE_BROWSER_UI_BROWSER_DIALOGS_H_

#include "base/callback_forward.h"

class Browser;
class SkBitmap;

namespace content {
class WebContents;
}  // namespace content

namespace brave {

// Tab restore dialog will be launched after ask dialog is closed.
void ShowCrashReportPermissionAskDialog(Browser* browser);

// Run |callback| when dialog closed.
void ShowObsoleteSystemConfirmDialog(base::OnceCallback<void(bool)> callback);

// Show web modal dialog for showing text that recognized from |image|.
void ShowTextRecognitionDialog(content::WebContents* web_contents,
                               const SkBitmap& image);

}  // namespace brave

#endif  // BRAVE_BROWSER_UI_BROWSER_DIALOGS_H_
