// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_COMMON_UI_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_COMMON_UI_H_

#include <stdint.h>

class ChromeBrowserState;

namespace url {
class Origin;
}  // namespace url

namespace brave_wallet {

void AddBlockchainTokenImageSource(ChromeBrowserState* browser_state);
bool IsBraveWalletOrigin(const url::Origin& origin);

}  // namespace brave_wallet

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_COMMON_UI_H_
