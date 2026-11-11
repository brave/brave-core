/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletDAppsActivity;

/**
 * Contract between a DApps panel, {@link BraveWalletDAppsActivity}, and the fragment it hosts. The
 * fragment gets the shared {@link WalletModel} from it and tells it what to show next.
 */
@NullMarked
public interface WalletFragmentCallback {
    WalletModel getWalletModel();

    /** Replaces the fragment on screen with the one for {@code activityType}. */
    void showPanel(final BraveWalletDAppsActivity.ActivityType activityType);

    /** Closes the panel, reporting that every request it was showing has been handled. */
    void onAllRequestsHandled();
}
