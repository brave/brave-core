/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.listeners;

import org.chromium.chrome.browser.crypto_wallet.model.WalletListItemModel;

public interface OnWalletListItemClick {
    default public void onAccountClick(WalletListItemModel walletListItemModel){};
    default public void onAssetClick(){};
    default public void onTransactionClick(){};
}
