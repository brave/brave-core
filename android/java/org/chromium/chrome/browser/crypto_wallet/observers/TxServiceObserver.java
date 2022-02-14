/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.observers;

import org.chromium.brave_wallet.mojom.TransactionInfo;

public interface TxServiceObserver
        extends org.chromium.brave_wallet.mojom.TxServiceObserver, WalletMojoObserverBase {
    @Override
    default void onNewUnapprovedTx(TransactionInfo txInfo) {}

    @Override
    default void onUnapprovedTxUpdated(TransactionInfo txInfo) {}

    @Override
    default void onTransactionStatusChanged(TransactionInfo txInfo) {}
}
