/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.observers;

import org.chromium.mojo.bindings.Interface;
import org.chromium.mojo.system.MojoException;

public interface WalletMojoObserverBase extends Interface {
    @Override
    default void close() {}

    @Override
    default void onConnectionError(MojoException e) {}
}
