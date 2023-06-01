/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import android.content.Context;

import androidx.lifecycle.LiveData;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.chrome.browser.crypto_wallet.model.CryptoAccountTypeInfo;

import java.util.List;

public interface CryptoSharedData {
    int getCoinType();
    String getChainId();
    Context getContext();
    LiveData<Integer> getCoinTypeLd();
    List<CryptoAccountTypeInfo> getSupportedCryptoAccountTypes();
    List<Integer> getSupportedCryptoCoins();
    LiveData<List<AccountInfo>> getAccounts();
}
