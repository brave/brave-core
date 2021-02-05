/** Copyright (c) 2020 The Brave Authors. All rights reserved.
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this file,
  * You can obtain one at http://mozilla.org/MPL/2.0/.
  */
package org.chromium.chrome.browser;

import androidx.annotation.VisibleForTesting;
import java.util.Iterator;
import java.util.Map;
import java.util.HashMap;

import org.json.JSONException;
import org.json.JSONObject;

public class BraveRewardsBalance {
    public static final String WALLET_ANONYMOUS = "anonymous";
    public static final String WALLET_UPHOLD = "uphold";
    public static final String WALLET_BLINDED = "blinded";

    /**
     * matching consts in
     * src/brave/components/brave_rewards/browser/balance.h
     */
    public static final String JSON_TOTAL = "total";
    public static final String JSON_WALLETS = "wallets";

    double mTotal;
    Map <String, Double> mWallets;

    BraveRewardsBalance (String json_balance) throws JSONException {
        fromJson (json_balance);
    }

    private void fromJson(String json_balance) throws JSONException {
        JSONObject jsonroot = new JSONObject(json_balance);
        mTotal = jsonroot.getDouble(JSON_TOTAL);

        mWallets = new HashMap <>();
        JSONObject json_wallets = jsonroot.getJSONObject(JSON_WALLETS);
        Iterator<String> keys = json_wallets.keys();
        while(keys.hasNext()) {
            String key = keys.next();
            Double val = json_wallets.getDouble(key);
            mWallets.put (key, val);
        }
    }

    public double getTotal() {
        return mTotal;
    }

    @VisibleForTesting
    @Override
    public String toString() {
        return "BraveRewardsBalance{" +
                "mTotal=" + mTotal +
                ", mWallets=" + mWallets +'}';
    }
}