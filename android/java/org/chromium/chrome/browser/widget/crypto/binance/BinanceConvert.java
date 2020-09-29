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

import org.chromium.base.Log;
import org.chromium.chrome.browser.widget.crypto.binance.ConvertAsset;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

public class BinanceConvert {
    private Map<String, List<ConvertAsset>> mCurrencyValues;

    public BinanceConvert(String json_balance) throws JSONException {
        fromJson(json_balance);
    }

    private void fromJson(String json_balance) throws JSONException {
        JSONObject jsonroot = new JSONObject(json_balance);
        Iterator<String> keys = jsonroot.keys();
        mCurrencyValues = new HashMap<>();
        while (keys.hasNext()) {
            String key = keys.next();
            JSONArray data = jsonroot.getJSONArray(key);
            List<ConvertAsset> tempList = new ArrayList<>();
            for (int i = 0; i < data.length(); i++) {
                JSONObject convertAssetJsonObject = new JSONObject(data.getString(i));
                tempList.add(new ConvertAsset(convertAssetJsonObject.getString("asset"),
                        convertAssetJsonObject.getString("minAmount")));
            }
            mCurrencyValues.put(key, tempList);
        }
    }

    public List<ConvertAsset> getCurrencyValue(String currency) {
        if (mCurrencyValues.containsKey(currency) && mCurrencyValues.get(currency) != null) {
            return mCurrencyValues.get(currency);
        }
        return null;
    }

    public List<String> getCurrencyKeys() {
        List<String> tempList = new ArrayList<String>();
        for (String key : mCurrencyValues.keySet()) {
            tempList.add(key);
        }
        return tempList;
    }
}