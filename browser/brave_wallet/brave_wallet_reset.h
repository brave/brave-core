/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_RESET_H_
#define BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_RESET_H_

#include "content/public/browser/browser_context.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace brave_wallet {

void ResetWallet(content::BrowserContext* context);

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_RESET_H_
