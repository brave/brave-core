/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TEST_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TEST_UTILS_H_

namespace brave_wallet {

class TxStateManager;

void WaitForTxStateManagerInitialized(TxStateManager* tx_state_manager);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TEST_UTILS_H_
