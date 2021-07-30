/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_CONTEXT_UTILS_H_
#define BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_CONTEXT_UTILS_H_

namespace content {
class BrowserContext;
}

namespace brave_wallet {

bool IsAllowedForContext(content::BrowserContext* context);

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_CONTEXT_UTILS_H_
