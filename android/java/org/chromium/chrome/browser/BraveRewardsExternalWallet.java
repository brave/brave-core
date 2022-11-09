/** Copyright (c) 2020 The Brave Authors. All rights reserved.
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this file,
  * You can obtain one at http://mozilla.org/MPL/2.0/.
  */
package org.chromium.chrome.browser;

import android.content.Context;

import androidx.annotation.VisibleForTesting;

import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.ledger.mojom.WalletStatus;

public class BraveRewardsExternalWallet {
    //fields
    public static final String ACCOUNT_URL = "account_url";
    public static final String ADD_URL = "add_url";
    public static final String ADDRESS = "address";
    public static final String STATUS = "status";
    public static final String TOKEN = "token";
    public static final String TYPE = "type";
    public static final String USER_NAME = "user_name";
    public static final String WITHDRAW_URL = "withdraw_url";
    public static final String LOGIN_URL = "login_url";

    private String mAccountUrl;
    private String mAddUrl;
    private String mAddress;
    private int mStatus;
    private String mToken;
    private String mType;
    private String mUserName;
    private String mWithdrawUrl;
    private String mLoginUrl;

    public String getAccountUrl() {
        return mAccountUrl;
    }

    public String getAddUrl() {
        return mAddUrl;
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

    public String getWithdrawUrl() {
        return mWithdrawUrl;
    }

    public String getLoginUrl() {
        return mLoginUrl;
    }

    public BraveRewardsExternalWallet(String json_external_wallet) throws JSONException {
        fromJson (json_external_wallet);
    }

    private void fromJson(String json_external_wallet) throws JSONException {
        JSONObject jsonObj = new JSONObject(json_external_wallet);
        mAccountUrl = jsonObj.getString(ACCOUNT_URL);
        mAddUrl = jsonObj.getString(ADD_URL);
        mAddress = jsonObj.getString(ADDRESS);
        mStatus = jsonObj.getInt(STATUS);
        mToken = jsonObj.getString(TOKEN);
        mType = jsonObj.getString(TYPE);
        mUserName = jsonObj.getString(USER_NAME);
        mWithdrawUrl = jsonObj.getString(WITHDRAW_URL);
        mLoginUrl = jsonObj.getString(LOGIN_URL);
    }

    @VisibleForTesting
    @Override
    public String toString() {
        return "BraveRewardsExternalWallet{"
                + "mAccountUrl='" + mAccountUrl + '\'' + ", mAddUrl='" + mAddUrl + '\''
                + ", mAddress='" + mAddress + '\'' + ", mStatus=" + mStatus + ", mToken='" + mToken
                + '\'' + ", mUserName='" + mUserName + '\'' + ", mWithdrawUrl='" + mWithdrawUrl
                + '\'' + ", mLoginUrl='" + mLoginUrl + '\'' + '}';
    }

    public static String WalletStatusToString(int status) {
        String value = "";
        Context context = ContextUtils.getApplicationContext();
        switch (status){
            case WalletStatus.NOT_CONNECTED:
                value = context.getResources().getString(
                    R.string.user_wallet_status_not_connected);
                break;
            case WalletStatus.CONNECTED:
                value = context.getResources().getString(
                    R.string.user_wallet_status_verified);
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
