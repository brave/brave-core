/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.observers;

import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletActivity;
import org.chromium.mojo.system.MojoException;

public interface KeyringServiceObserver
        extends org.chromium.brave_wallet.mojom.KeyringServiceObserver {
    @Override
    default void keyringCreated() {}

    @Override
    default void keyringRestored() {}

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

    @Override
    default void close() {}

    @Override
    default void onConnectionError(MojoException e) {}
}
