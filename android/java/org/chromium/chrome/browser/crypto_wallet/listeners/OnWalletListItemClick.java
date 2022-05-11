/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.listeners;

import android.widget.CheckBox;

import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.chrome.browser.crypto_wallet.model.WalletListItemModel;

public interface OnWalletListItemClick {
    default public void onAccountClick(WalletListItemModel walletListItemModel){};
    default public void onAssetClick(BlockchainToken asset){};
    default public void onTransactionClick(TransactionInfo txInfo){};
    default public void onAssetCheckedChanged(
            WalletListItemModel walletListItemModel, CheckBox assetCheck, boolean isChecked){};
    default public void onTrashIconClick(WalletListItemModel walletListItemModel){};
}
