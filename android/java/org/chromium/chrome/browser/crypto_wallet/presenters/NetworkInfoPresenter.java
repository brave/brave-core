/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.presenters;

import org.chromium.brave_wallet.mojom.NetworkInfo;

import java.util.List;

public class NetworkInfoPresenter {
    public NetworkInfo mNetworkInfo;
    public boolean mIsPrimary;
    public List<NetworkInfo> mSubNetworks;

    public NetworkInfoPresenter(
            NetworkInfo mNetworkInfo, boolean mIsPrimary, List<NetworkInfo> mSubNetworks) {
        this.mNetworkInfo = mNetworkInfo;
        this.mIsPrimary = mIsPrimary;
        this.mSubNetworks = mSubNetworks;
    }
}
