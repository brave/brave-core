/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DIALOG_OPENER_H_
#define BRAVE_BROWSER_UI_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DIALOG_OPENER_H_

#include <string>

#include "brave/components/brave_account/mojom/brave_account.mojom.h"

namespace content {
class WebContents;
}

namespace brave_account {

void OpenBraveAccountDialog(content::WebContents& web_contents,
                            const std::string& initiating_service_name,
                            mojom::DialogMode dialog_mode);

}  // namespace brave_account

#endif  // BRAVE_BROWSER_UI_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DIALOG_OPENER_H_
