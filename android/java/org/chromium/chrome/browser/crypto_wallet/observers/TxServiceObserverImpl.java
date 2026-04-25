/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.observers;

import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TxServiceObserver;
import org.chromium.mojo.system.MojoException;

public class TxServiceObserverImpl implements TxServiceObserver {
    public interface TxServiceObserverImplDelegate {
        default void onNewUnapprovedTx(TransactionInfo txInfo) {}

        default void onUnapprovedTxUpdated(TransactionInfo txInfo) {}

        default void onTransactionStatusChanged(TransactionInfo txInfo) {}
    }

    private TxServiceObserverImplDelegate mDelegate;

    public TxServiceObserverImpl(TxServiceObserverImplDelegate delegate) {
        mDelegate = delegate;
    }

    @Override
    public void onNewUnapprovedTx(TransactionInfo txInfo) {
        if (mDelegate == null) return;

        mDelegate.onNewUnapprovedTx(txInfo);
    }

    @Override
    public void onUnapprovedTxUpdated(TransactionInfo txInfo) {
        if (mDelegate == null) return;

        mDelegate.onUnapprovedTxUpdated(txInfo);
    }

    @Override
    public void onTransactionStatusChanged(TransactionInfo txInfo) {
        if (mDelegate == null) return;

        mDelegate.onTransactionStatusChanged(txInfo);
    }

    @Override
    public void onTxServiceReset() {}

    @Override
    public void close() {
        mDelegate = null;
    }

    @Override
    public void onConnectionError(MojoException e) {}

    public void destroy() {
        mDelegate = null;
    }
}
