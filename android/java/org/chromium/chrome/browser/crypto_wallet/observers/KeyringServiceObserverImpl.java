/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.observers;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.KeyringServiceObserver;
import org.chromium.mojo.system.MojoException;

import java.lang.ref.WeakReference;

public class KeyringServiceObserverImpl implements KeyringServiceObserver {
    public interface KeyringServiceObserverImplDelegate {
        default void locked() {}
        default void backedUp() {}
        default void keyringCreated(String keyringId) {}
        default void keyringRestored(String keyringId) {}
        default void keyringReset() {}
        default void unlocked() {}
        default void accountsChanged() {}
        default void accountsAdded(AccountInfo[] addedAccounts) {}
        default void autoLockMinutesChanged() {}
        default void selectedAccountChanged(AccountInfo selectedAccount) {}
        default void selectedDappAccountChangedForCoin(int coin) {}
    }

    private WeakReference<KeyringServiceObserverImplDelegate> mDelegate;

    public KeyringServiceObserverImpl(KeyringServiceObserverImplDelegate delegate) {
        mDelegate = new WeakReference<>(delegate);
    }

    @Override
    public void keyringCreated(String keyringId) {
        if (isActive()) getRef().keyringCreated(keyringId);
    }

    @Override
    public void keyringRestored(String keyringId) {
        if (isActive()) getRef().keyringRestored(keyringId);
    }

    @Override
    public void keyringReset() {
        if (isActive()) getRef().keyringReset();
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
    public void selectedAccountChanged(AccountInfo selectedAccount) {
        if (isActive()) getRef().selectedAccountChanged(selectedAccount);
    }

    @Override
    public void selectedDappAccountChangedForCoin(int coin) {}

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
