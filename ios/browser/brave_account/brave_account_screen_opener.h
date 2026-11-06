/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SCREEN_OPENER_H_
#define BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SCREEN_OPENER_H_

#include "brave/components/brave_account/mojom/brave_account.mojom-forward.h"

namespace web {
class WebState;
}

namespace brave_account {

// Ask the host surface to open a screen, through the
// `BraveAccountScreenOpenerBridge` it set on `web_state`. Both do nothing if no
// bridge was set or the weak bridge is gone.

// Opens the Brave Account dialog over `web_state`.
void OpenBraveAccountDialog(web::WebState& web_state,
                            mojom::DialogMode dialog_mode);

// Opens the settings screen for `page` in place of `web_state`.
void OpenSettingsPage(web::WebState& web_state, mojom::SettingsPage page);

}  // namespace brave_account

#endif  // BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SCREEN_OPENER_H_
