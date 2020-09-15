/** Copyright (c) 2020 The Brave Authors. All rights reserved.
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this file,
  * You can obtain one at http://mozilla.org/MPL/2.0/.
  */
package org.chromium.chrome.browser.widget.crypto.binance;

import androidx.annotation.VisibleForTesting;
import java.util.Iterator;
import java.util.Map;
import java.util.HashMap;

import android.util.Pair;
import java.util.List;
import java.util.ArrayList;

import org.json.JSONException;
import org.json.JSONObject;
import org.json.JSONArray;

public class BinanceConvert {
    private Map <String, List<String>> mCurrencyValues;

    public BinanceConvert (String json_balance) throws JSONException {
        fromJson (json_balance);
    }

    private void fromJson(String json_balance) throws JSONException {
        JSONObject jsonroot = new JSONObject(json_balance);
        Iterator<String> keys = jsonroot.keys();
        mCurrencyValues = new HashMap <>();
        while (keys.hasNext()) {
            String key = keys.next();
            JSONArray data = jsonroot.getJSONArray(key);
            List<String> tempList = new ArrayList<String>();
            for (int i = 0; i < data.length(); i++) {
                tempList.add(data.getString(i));
            }
            mCurrencyValues.put(key, tempList);
        }
    }

    public List<String> getCurrencyValue(String currency) {
        if (mCurrencyValues.containsKey(currency)
                && mCurrencyValues.get(currency) != null) {
            return mCurrencyValues.get(currency);
        }
        return null;
    }

    public List<String> getCurrencyKeys() {
        List<String> tempList = new ArrayList<String>();
        for ( String key : mCurrencyValues.keySet() ) {
            tempList.add(key);
        }
        return tempList;
    }
}