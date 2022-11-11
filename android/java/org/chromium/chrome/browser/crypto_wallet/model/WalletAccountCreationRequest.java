/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.model;

import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.chrome.browser.crypto_wallet.util.AssetUtils;

public class WalletAccountCreationRequest {
    private String mKeyringId;
    private @CoinType.EnumType int mCoinType;

    public WalletAccountCreationRequest(String keyringId) {
        assert keyringId != null : " keyringId should not be null";
        mKeyringId = keyringId;
        mCoinType = AssetUtils.getCoinForKeyring(keyringId);
    }

    public String getKeyringId() {
        return mKeyringId;
    }

    public int getCoinType() {
        return mCoinType;
    }
}
