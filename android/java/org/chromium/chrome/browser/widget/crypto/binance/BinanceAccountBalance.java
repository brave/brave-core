/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */
package org.chromium.chrome.browser.widget.crypto.binance;

import android.util.Pair;

import androidx.annotation.VisibleForTesting;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

public class BinanceAccountBalance {
    private double mTotalUSD;
    private double mTotalBTC;
    private Map<String, Pair<Double, Double>>
            mCurrencyValues; // First : Currency value, Second : USD value

    public BinanceAccountBalance(String json_balance) throws JSONException {
        fromJson(json_balance);
    }

    private void fromJson(String json_balance) throws JSONException {
        JSONObject jsonroot = new JSONObject(json_balance);
        Iterator<String> keys = jsonroot.keys();
        mCurrencyValues = new HashMap<>();
        while (keys.hasNext()) {
            String key = keys.next();
            JSONArray data = jsonroot.getJSONArray(key);
            mTotalBTC = mTotalBTC + data.getDouble(1);
            mTotalUSD = mTotalUSD + data.getDouble(2);
            mCurrencyValues.put(key, new Pair<>(data.getDouble(0), data.getDouble(2)));
        }
    }

    @VisibleForTesting
    @Override
    public String toString() {
        return "BinanceAccountBalance{"
                + "TotalUSD=" + mTotalUSD + ", TotalBTC=" + mTotalBTC + '}';
    }

    public double getTotalUSD() {
        return mTotalUSD;
    }

    public double getTotalBTC() {
        return mTotalBTC;
    }

    public Pair<Double, Double> getCurrencyValue(String currency) {
        if (mCurrencyValues.containsKey(currency) && mCurrencyValues.get(currency) != null) {
            return mCurrencyValues.get(currency);
        }
        return null;
    }
}