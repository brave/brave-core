/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */
package org.chromium.chrome.browser.widget.crypto.binance;

import android.text.TextUtils;
import android.util.Pair;

import androidx.annotation.VisibleForTesting;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.Log;
import org.chromium.chrome.browser.widget.crypto.binance.CoinNetworkModel;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

public class BinanceCoinNetworks {
    private List<CoinNetworkModel> mCoinNetwroksList = new ArrayList<>();
    public BinanceCoinNetworks(String jsonNetworks) throws JSONException {
        fromJson(jsonNetworks);
    }

    private void fromJson(String jsonNetworks) throws JSONException {
        JSONObject jsonroot = new JSONObject(jsonNetworks);
        Iterator<String> keys = jsonroot.keys();
        while (keys.hasNext()) {
            String key = keys.next();
            if (BinanceWidgetManager.comCurrenciesMap.containsKey(key)) {
                CoinNetworkModel coinNetworkModel = BinanceWidgetManager.comCurrenciesMap.get(key);
                coinNetworkModel.setTickerNetwork(jsonroot.getString(key));
                mCoinNetwroksList.add(coinNetworkModel);
            }
        }
    }

    public List<CoinNetworkModel> getCoinNetworksList() {
        return mCoinNetwroksList;
    }

    @Override
    public String toString() {
        return TextUtils.join(", ", mCoinNetwroksList);
    }
}