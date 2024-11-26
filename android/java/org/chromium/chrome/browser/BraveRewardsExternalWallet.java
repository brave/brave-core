/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */
package org.chromium.chrome.browser;

import android.content.Context;

import androidx.annotation.VisibleForTesting;

import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.ContextUtils;
import org.chromium.brave_rewards.mojom.WalletStatus;
import org.chromium.chrome.R;

public class BraveRewardsExternalWallet {
    //fields
    public static final String ACCOUNT_URL = "account_url";
    public static final String ADDRESS = "address";
    public static final String STATUS = "status";
    public static final String TOKEN = "token";
    public static final String TYPE = "type";
    public static final String USER_NAME = "user_name";

    private String mAccountUrl;
    private String mAddress;
    private int mStatus;
    private String mToken;
    private String mType;
    private String mUserName;

    public String getAccountUrl() {
        return mAccountUrl;
    }

    public String getAddress() {
        return mAddress;
    }

    public int getStatus() {
        return mStatus;
    }

    public String getToken() {
        return mToken;
    }

    public String getType() {
        return mType;
    }

    public String getUserName() {
        return mUserName;
    }

    public BraveRewardsExternalWallet(String json_external_wallet) throws JSONException {
        fromJson (json_external_wallet);
    }

    private void fromJson(String json_external_wallet) throws JSONException {
        JSONObject jsonObj = new JSONObject(json_external_wallet);
        mAccountUrl = jsonObj.getString(ACCOUNT_URL);
        mAddress = jsonObj.getString(ADDRESS);
        mStatus = jsonObj.getInt(STATUS);
        mToken = jsonObj.getString(TOKEN);
        mType = jsonObj.getString(TYPE);
        mUserName = jsonObj.getString(USER_NAME);
    }

    @VisibleForTesting
    @Override
    public String toString() {
        return "BraveRewardsExternalWallet{"
                + "mAccountUrl='"
                + mAccountUrl
                + '\''
                + ", mAddress='"
                + mAddress
                + '\''
                + ", mStatus="
                + mStatus
                + ", mToken='"
                + mToken
                + '\''
                + ", mUserName='"
                + mUserName
                + '\''
                + '}';
    }

    public static String walletStatusToString(int status) {
        String value = "";
        Context context = ContextUtils.getApplicationContext();
        switch (status) {
            case WalletStatus.NOT_CONNECTED:
                value = context.getResources().getString(R.string.user_wallet_status_not_connected);
                break;
            case WalletStatus.CONNECTED:
                value = context.getResources().getString(R.string.brave_ui_wallet_button_connected);
                break;
            case WalletStatus.LOGGED_OUT:
                value = context.getResources().getString(
                        R.string.brave_ui_wallet_button_logged_out);
                break;
            default:
                break;
        }
        return value;
    }
}
