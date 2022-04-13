/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.listeners;

public interface TransactionConfirmationListener {
    void onNextTransaction();
    void onConfirmTransaction();
    void onRejectTransaction();
    void onRejectAllTransactions();
    void onDismiss();
}
