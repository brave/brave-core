/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.listeners;

import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.build.annotations.Nullable;

import java.util.List;

public interface TransactionListener {
    void onNextTransaction();

    void onRejectAllTransactions();

    void onCancel();

    List<TransactionInfo> getPendingTransactions();

    @Nullable
    TransactionInfo getCurrentTransaction();
}
