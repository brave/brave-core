/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.observers;

public interface KeyringServiceObserver
        extends org.chromium.brave_wallet.mojom.KeyringServiceObserver, WalletMojoObserverBase {
    @Override
    default void keyringCreated(String keyring_id) {}

    @Override
    default void keyringRestored(String keyring_id) {}

    @Override
    default void keyringReset() {}

    @Override
    default void locked() {}

    @Override
    default void unlocked() {}

    @Override
    default void backedUp() {}

    @Override
    default void accountsChanged() {}

    @Override
    default void autoLockMinutesChanged() {}

    @Override
    default void selectedAccountChanged() {}
}
