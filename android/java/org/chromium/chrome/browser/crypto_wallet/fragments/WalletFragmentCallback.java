/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletDAppsActivity;

/**
 * Contract that exposes the shared {@link WalletModel} to wallet fragments so they can access
 * services and state managed by the hosting activity {@link BraveWalletDAppsActivity}.
 */
@NullMarked
public interface WalletFragmentCallback {
    WalletModel getWalletModel();
}
