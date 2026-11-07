/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SETTINGS_OPENER_H_
#define BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SETTINGS_OPENER_H_

#include "brave/components/brave_account/mojom/brave_account.mojom-forward.h"

namespace web {
class WebState;
}

namespace brave_account {

// Asks the host surface to open the settings screen for `page` in place of
// `web_state`, through the `BraveAccountSettingsOpenerBridge` it set on
// `web_state`. Does nothing if no bridge was set or the weak bridge is gone.
void OpenSettingsPage(web::WebState& web_state, mojom::SettingsPage page);

}  // namespace brave_account

#endif  // BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SETTINGS_OPENER_H_
