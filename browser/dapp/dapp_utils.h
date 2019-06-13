/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_DAPP_DAPP_UTILS_H_
#define BRAVE_BROWSER_DAPP_DAPP_UTILS_H_

#include "base/callback_forward.h"

namespace content {
class BrowserContext;
class WebContents;
}  // content

bool DappDetectionEnabled(content::BrowserContext* browser_context);
void RequestWalletInstallationPermission(content::WebContents* web_contents);

void SetQuitClosureForDappDetectionTest(const base::Closure& quit_closure);

#endif  // BRAVE_BROWSER_DAPP_DAPP_UTILS_H_
