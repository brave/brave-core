/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.listeners;

import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletDAppsActivity;

import java.util.List;

/**
 * Listener that exposes wallet transactions and actions to approval UI components.
 * Implemented by host activity {@link BraveWalletDAppsActivity} so fragments can step through
 * pending transactions and react to user decisions.
 */
@NullMarked
public interface TransactionListener {
    void onNextTransaction();

    void onRejectAllTransactions();

    void onCancel();

    List<TransactionInfo> getPendingTransactions();

    @Nullable TransactionInfo getCurrentTransaction();
}
