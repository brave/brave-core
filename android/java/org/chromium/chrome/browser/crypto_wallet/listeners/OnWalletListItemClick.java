/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.listeners;

import android.widget.CheckBox;
import android.widget.ImageView;

import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.chrome.browser.crypto_wallet.model.WalletListItemModel;

public interface OnWalletListItemClick {
    default void onAccountClick(WalletListItemModel walletListItemModel) {}
    default void onAssetClick(BlockchainToken asset) {}
    default void onTransactionClick(TransactionInfo txInfo) {}
    default void onAssetCheckedChanged(
            WalletListItemModel walletListItemModel, CheckBox assetCheck, boolean isChecked) {}
    default void onMaybeShowTrashButton(
            WalletListItemModel walletListItemModel, ImageView trashButton) {}
    default void onTrashIconClick(WalletListItemModel walletListItemModel) {}
}
