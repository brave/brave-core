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

    public String mAccount_url;
    public String mAdd_url;
    public String mAddress;
    @WalletStatus public int mStatus;
    public String mToken;
    public String mType;
    public String mUser_name;
    public String mVerify_url;
    public String mWithdraw_url;


    BraveRewardsExternalWallet (String json_external_wallet) throws JSONException {
        fromJson (json_external_wallet);
    }

    private void fromJson(String json_external_wallet) throws JSONException {
        JSONObject jsonObj = new JSONObject(json_external_wallet);
        mAccount_url = jsonObj.getString(ACCOUNT_URL);
        mAdd_url = jsonObj.getString(ADD_URL);
        mAddress = jsonObj.getString(ADDRESS);
        mStatus = jsonObj.getInt(STATUS);
        mToken = jsonObj.getString(TOKEN);
        mType = jsonObj.getString(TYPE);
        mUser_name = jsonObj.getString(USER_NAME);
        mVerify_url = jsonObj.getString(VERIFY_URL);
        mWithdraw_url = jsonObj.getString(WITHDRAW_URL);
    }

    @VisibleForTesting
    @Override
    public String toString() {
        return "BraveRewardsExternalWallet{" +
                "mAccount_url='" + mAccount_url + '\'' +
                ", mAdd_url='" + mAdd_url + '\'' +
                ", mAddress='" + mAddress + '\'' +
                ", mStatus=" + mStatus +
                ", mToken='" + mToken + '\'' +
                ", mType='" + mType + '\'' +
                ", mUser_name='" + mUser_name + '\'' +
                ", mVerify_url='" + mVerify_url + '\'' +
                ", mWithdraw_url='" + mWithdraw_url + '\'' +
                '}';
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