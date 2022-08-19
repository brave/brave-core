/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.observers;

import org.chromium.mojo.system.MojoException;

public class KeyringServiceObserverImpl
        implements org.chromium.brave_wallet.mojom.KeyringServiceObserver {
    public interface KeyringServiceObserverImplDelegate {
        default void locked() {}
        default void backedUp() {}
    }

    private KeyringServiceObserverImplDelegate mDelegate;

    public KeyringServiceObserverImpl(KeyringServiceObserverImplDelegate delegate) {
        mDelegate = delegate;
    }

    @Override
    public void keyringCreated(String keyring_id) {}

    @Override
    public void keyringRestored(String keyring_id) {}

    @Override
    public void keyringReset() {}

    @Override
    public void locked() {
        if (mDelegate == null) return;

        mDelegate.locked();
    }

    @Override
    public void unlocked() {}

    @Override
    public void backedUp() {
        if (mDelegate == null) return;

        mDelegate.backedUp();
    }

    @Override
    public void accountsChanged() {}

    @Override
    public void autoLockMinutesChanged() {}

    @Override
    public void selectedAccountChanged(int coin) {}

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
