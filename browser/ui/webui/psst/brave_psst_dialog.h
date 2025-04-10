/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

 #ifndef BRAVE_BROWSER_UI_WEBUI_PSST_BRAVE_PSST_DIALOG_H_
 #define BRAVE_BROWSER_UI_WEBUI_PSST_BRAVE_PSST_DIALOG_H_

#include "content/public/browser/web_contents.h"

namespace psst {

void OpenPsstDialog(content::WebContents* initiator);

}  // namespace psst

 #endif  // BRAVE_BROWSER_UI_WEBUI_PSST_BRAVE_PSST_DIALOG_H_