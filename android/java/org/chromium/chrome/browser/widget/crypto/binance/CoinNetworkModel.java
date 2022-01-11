/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */
package org.chromium.chrome.browser.widget.crypto.binance;

import androidx.annotation.VisibleForTesting;

public class CoinNetworkModel {
    private String coin;
    private String tickerNetwork;
    private String coinDesc;
    private int coinRes;

    public CoinNetworkModel(String coin, String coinDesc, int coinRes) {
        this.coin = coin;
        this.coinDesc = coinDesc;
        this.coinRes = coinRes;
    }

    public void setTickerNetwork(String tickerNetwork) {
        this.tickerNetwork = tickerNetwork;
    }

    public String getCoin() {
        return coin;
    }

    public String getTickerNetwork() {
        return tickerNetwork;
    }

    public String getCoinDesc() {
        return coinDesc;
    }

    public int getCoinRes() {
        return coinRes;
    }

    @Override
    public String toString() {
        StringBuilder strBuilder = new StringBuilder(coin);
        strBuilder.append(tickerNetwork);
        strBuilder.append(coinDesc);
        return strBuilder.toString();
    }
}