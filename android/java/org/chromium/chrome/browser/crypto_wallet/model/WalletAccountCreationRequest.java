/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.model;

import org.chromium.brave_wallet.mojom.CoinType;

public class WalletAccountCreationRequest {
    private @CoinType.EnumType int mCoinType;

    public WalletAccountCreationRequest(@CoinType.EnumType int coinType) {
        mCoinType = coinType;
    }

    public int getCoinType() {
        return mCoinType;
    }
}
