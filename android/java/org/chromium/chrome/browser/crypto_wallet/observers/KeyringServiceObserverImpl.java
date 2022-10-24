/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.observers;

import org.chromium.brave_wallet.mojom.KeyringServiceObserver;
import org.chromium.mojo.system.MojoException;

public class KeyringServiceObserverImpl implements KeyringServiceObserver {
    public interface KeyringServiceObserverImplDelegate {
        default void locked() {}
        default void backedUp() {}
        default void keyringCreated(String keyringId) {}
        default void keyringRestored(String keyringId) {}
        default void keyringReset() {}
        default void unlocked() {}
        default void accountsChanged() {}
        default void accountsAdded(int coin, String[] addresses) {}
        default void autoLockMinutesChanged() {}
        default void selectedAccountChanged(int coin) {}
    }

    private KeyringServiceObserverImplDelegate mDelegate;

    public KeyringServiceObserverImpl(KeyringServiceObserverImplDelegate delegate) {
        mDelegate = delegate;
    }

    @Override
    public void keyringCreated(String keyringId) {
        if (mDelegate == null) return;

        mDelegate.keyringCreated(keyringId);
    }

    @Override
    public void keyringRestored(String keyringId) {
        if (mDelegate == null) return;

        mDelegate.keyringRestored(keyringId);
    }

    @Override
    public void keyringReset() {
        if (mDelegate == null) return;

        mDelegate.keyringReset();
    }

    @Override
    public void locked() {
        if (mDelegate == null) return;

        mDelegate.locked();
    }

    @Override
    public void unlocked() {
        if (mDelegate == null) return;

        mDelegate.unlocked();
    }

    @Override
    public void backedUp() {
        if (mDelegate == null) return;

        mDelegate.backedUp();
    }

    @Override
    public void accountsChanged() {
        if (mDelegate == null) return;

        mDelegate.accountsChanged();
    }

    @Override
    public void accountsAdded(int coin, String[] addresses) {
        if (mDelegate == null) return;

        mDelegate.accountsAdded(coin, addresses);
    }

    @Override
    public void autoLockMinutesChanged() {
        if (mDelegate == null) return;

        mDelegate.autoLockMinutesChanged();
    }

    @Override
    public void selectedAccountChanged(int coin) {
        if (mDelegate == null) return;

        mDelegate.selectedAccountChanged(coin);
    }

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
