/** Copyright (c) 2020 The Brave Authors. All rights reserved.
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this file,
  * You can obtain one at http://mozilla.org/MPL/2.0/.
  */
package org.chromium.chrome.browser;

import android.content.Context;
import androidx.annotation.IntDef;
import androidx.annotation.VisibleForTesting;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.json.JSONException;
import org.json.JSONObject;

class BraveRewardsExternalWallet {
    //fields
    public static final String ACCOUNT_URL = "account_url";
    public static final String ADD_URL = "add_url";
    public static final String ADDRESS = "address";
    public static final String STATUS = "status";
    public static final String TOKEN = "token";
    public static final String TYPE = "type";
    public static final String USER_NAME = "user_name";
    public static final String VERIFY_URL = "verify_url";
    public static final String WITHDRAW_URL = "withdraw_url";
    public static final String LOGIN_URL = "login_url";

    //WalletStatus @
    //vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger.mojom
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({NOT_CONNECTED, CONNECTED, VERIFIED, DISCONNECTED_NOT_VERIFIED,
            DISCONNECTED_VERIFIED, PENDING})
    public @interface WalletStatus {}
    public static final int NOT_CONNECTED = 0;
    public static final int CONNECTED = 1;
    public static final int VERIFIED = 2;
    public static final int DISCONNECTED_NOT_VERIFIED = 3;
    public static final int DISCONNECTED_VERIFIED = 4;
    public static final int PENDING = 5;

    private String mAccountUrl;
    private String mAddUrl;
    private String mAddress;
    @WalletStatus
    private int mStatus;
    private String mToken;
    private String mType;
    private String mUserName;
    private String mVerifyUrl;
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

    public String getVerifyUrl() {
        return mVerifyUrl;
    }

    public String getWithdrawUrl() {
        return mWithdrawUrl;
    }

    public String getLoginUrl() {
        return mLoginUrl;
    }

    BraveRewardsExternalWallet (String json_external_wallet) throws JSONException {
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
        mVerifyUrl = jsonObj.getString(VERIFY_URL);
        mWithdrawUrl = jsonObj.getString(WITHDRAW_URL);
        mLoginUrl = jsonObj.getString(LOGIN_URL);
    }

    @VisibleForTesting
    @Override
    public String toString() {
        return "BraveRewardsExternalWallet{"
                + "mAccountUrl='" + mAccountUrl + '\'' + ", mAddUrl='" + mAddUrl + '\''
                + ", mAddress='" + mAddress + '\'' + ", mStatus=" + mStatus + ", mToken='" + mToken
                + '\'' + ", mUserName='" + mUserName + '\'' + ", mVerifyUrl='" + mVerifyUrl + '\''
                + ", mWithdrawUrl='" + mWithdrawUrl + '\'' + ", mLoginUrl='" + mLoginUrl + '\''
                + '}';
    }

    public static String WalletStatusToString (@WalletStatus int status){
        String value = "";
        Context context = ContextUtils.getApplicationContext();
        switch (status){
            case NOT_CONNECTED:
                value = context.getResources().getString(
                    R.string.user_wallet_status_not_connected);
                break;
            case CONNECTED:
                value = context.getResources().getString(
                    R.string.user_wallet_status_connected);
                break;
            case VERIFIED:
                value = context.getResources().getString(
                    R.string.user_wallet_status_verified);
                break;
            case DISCONNECTED_NOT_VERIFIED:
                value = context.getResources().getString(
                    R.string.user_wallet_status_disconnected_not_verified);
                break;
            case DISCONNECTED_VERIFIED:
                value = context.getResources().getString(
                    R.string.user_wallet_status_disconnected_verified);
                break;
            case PENDING:
                value = context.getResources().getString(
                    R.string.user_wallet_status_pending);
                break;
            default:
                break;
        }
        return value;
    }
}
