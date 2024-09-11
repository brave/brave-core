/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.observers;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.KeyringServiceObserver;
import org.chromium.mojo.system.MojoException;

import java.lang.ref.WeakReference;

public class KeyringServiceObserverImpl implements KeyringServiceObserver {
    public interface KeyringServiceObserverImplDelegate {
        default void locked() {}

        default void backedUp() {}

        default void walletCreated() {}

        default void walletRestored() {}

        default void walletReset() {}

        default void unlocked() {}

        default void accountsChanged() {}

        default void accountsAdded(AccountInfo[] addedAccounts) {}

        default void autoLockMinutesChanged() {}

        default void selectedWalletAccountChanged(AccountInfo accountInfo) {}

        default void selectedDappAccountChanged(
                @CoinType.EnumType int coinType, AccountInfo accountInfo) {}
    }

    private WeakReference<KeyringServiceObserverImplDelegate> mDelegate;

    public KeyringServiceObserverImpl(KeyringServiceObserverImplDelegate delegate) {
        mDelegate = new WeakReference<>(delegate);
    }

    @Override
    public void walletCreated() {
        if (isActive()) getRef().walletCreated();
    }

    @Override
    public void walletRestored() {
        if (isActive()) getRef().walletRestored();
    }

    @Override
    public void walletReset() {
        if (isActive()) getRef().walletReset();
    }

    @Override
    public void locked() {
        if (isActive()) getRef().locked();
    }

    @Override
    public void unlocked() {
        if (isActive()) getRef().unlocked();
    }

    @Override
    public void backedUp() {
        if (isActive()) getRef().backedUp();
    }

    @Override
    public void accountsChanged() {
        if (isActive()) getRef().accountsChanged();
    }

    @Override
    public void accountsAdded(AccountInfo[] addedAccounts) {
        if (isActive()) getRef().accountsAdded(addedAccounts);
    }

    @Override
    public void autoLockMinutesChanged() {
        if (isActive()) getRef().autoLockMinutesChanged();
    }

    @Override
    public void selectedWalletAccountChanged(AccountInfo accountInfo) {
        if (isActive()) getRef().selectedWalletAccountChanged(accountInfo);
    }

    @Override
    public void selectedDappAccountChanged(
            @CoinType.EnumType int coinType, AccountInfo accountInfo) {
        if (isActive()) getRef().selectedDappAccountChanged(coinType, accountInfo);
    }

    @Override
    public void close() {
        mDelegate = null;
    }

    @Override
    public void onConnectionError(MojoException e) {}

    private KeyringServiceObserverImplDelegate getRef() {
        return mDelegate.get();
    }

    private boolean isActive() {
        return mDelegate != null && mDelegate.get() != null;
    }
}
