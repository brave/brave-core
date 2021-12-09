/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.widget.crypto.binance;

import android.content.SharedPreferences;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceAccountBalance;
import org.chromium.chrome.browser.widget.crypto.binance.CoinNetworkModel;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

public class BinanceWidgetManager {
    public static final String PREF_BINANCE_USER_AUTHENTICATION = "binance_user_authentication";
    public static final String PREF_BINANCE_ACCOUNT_BALANCE = "binance_account_balance";

    public static final String BINANCE_US =
            "https://www.binance.us/en/buy-sell-crypto?crypto=%1$s&amount=%2$s&ref=35089877&utm_source=brave";
    public static final String BINANCE_COM_ONE = "https://www.binance.com/";
    public static final String BINANCE_COM_TWO =
            "/buy-sell-crypto?fiat=%1$s&crypto=%2$s&amount=%3$s&ref=W4GYNUAJ&utm_source=brave";

    private static BinanceWidgetManager sInstance;

    private final SharedPreferences mSharedPreferences;

    public static BinanceAccountBalance binanceAccountBalance;

    public static final List<String> fiatList = Arrays.asList("USD", "AUD", "ARS", "BGN", "BRL",
            "CAD", "CHF", "CNY", "COP", "CZK", "DKK", "EUR", "GBP", "HKD", "HRK", "HUF", "IDR",
            "ILS", "INR", "JPY", "KRW", "KZT", "MXN", "NGN", "NOK", "NZD", "PLN", "RON", "RUB",
            "SEK", "THB", "TRY", "UAH", "VND", "ZAR");
    public static final Map<String, CoinNetworkModel> extraCurrenciesMap =
            new HashMap<String, CoinNetworkModel>() {
                {
                    put("XVG", new CoinNetworkModel("XVG", "", R.drawable.xvg));
                    put("DOGE", new CoinNetworkModel("DOGE", "", R.drawable.doge));
                    put("ETC", new CoinNetworkModel("ETC", "", R.drawable.etc));
                    put("ZEC", new CoinNetworkModel("ZEC", "", R.drawable.zec));
                    put("LTC", new CoinNetworkModel("LTC", "", R.drawable.ltc));
                    put("USDC", new CoinNetworkModel("USDC", "", R.drawable.usdc));
                    put("ENG", new CoinNetworkModel("ENG", "", R.drawable.eng));
                    put("ADX", new CoinNetworkModel("ADX", "", R.drawable.adx));
                }
            };
    public static final Map<String, CoinNetworkModel> usCurrenciesMap =
            new HashMap<String, CoinNetworkModel>() {
                {
                    put("BTC", new CoinNetworkModel("BTC", "Bitcoin", R.drawable.btc));
                    put("ETH", new CoinNetworkModel("ETH", "Ethereum", R.drawable.eth));
                    put("BNB", new CoinNetworkModel("BNB", "Binance Coin", R.drawable.bnb));
                    put("BCH", new CoinNetworkModel("BCH", "Bitcoin Cash", R.drawable.bch));
                    put("XRP", new CoinNetworkModel("XRP", "Ripple", R.drawable.xrp));
                    put("BUSD", new CoinNetworkModel("BUSD", "US Dollar", R.drawable.busd));
                    put("BAT",
                            new CoinNetworkModel("BAT", "Basic Attention Token", R.drawable.bat));
                    put("XTZ", new CoinNetworkModel("XTZ", "", R.drawable.xtz));
                    put("ALGO", new CoinNetworkModel("ALGO", "", 0));
                    put("ATOM", new CoinNetworkModel("ATOM", "", R.drawable.atom));
                    put("LINK", new CoinNetworkModel("LINK", "", R.drawable.link));
                    put("VET", new CoinNetworkModel("VET", "", R.drawable.vet));
                    put("ZRX", new CoinNetworkModel("ZRX", "", R.drawable.zrx));
                    put("ADA", new CoinNetworkModel("ADA", "", R.drawable.ada));
                    put("XLM", new CoinNetworkModel("XLM", "", R.drawable.xlm));
                    put("ENJ", new CoinNetworkModel("ENJ", "", R.drawable.enj));
                    put("ZIL", new CoinNetworkModel("ZIL", "", R.drawable.zil));
                    put("RVN", new CoinNetworkModel("RVN", "", R.drawable.rvn));
                    put("ONT", new CoinNetworkModel("ONT", "", R.drawable.ont));
                    put("HBAR", new CoinNetworkModel("HBAR", "", 0));
                    put("MATIC", new CoinNetworkModel("MATIC", "", R.drawable.matic));
                }
            };
    public static final Map<String, CoinNetworkModel> comCurrenciesMap =
            new HashMap<String, CoinNetworkModel>() {
                {
                    put("BTC", new CoinNetworkModel("BTC", "Bitcoin", R.drawable.btc));
                    put("ETH", new CoinNetworkModel("ETH", "Ethereum", R.drawable.eth));
                    put("BAT",
                            new CoinNetworkModel("BAT", "Basic Attention Token", R.drawable.bat));
                    put("XRP", new CoinNetworkModel("XRP", "Ripple", R.drawable.xrp));
                    put("BNB", new CoinNetworkModel("BNB", "Binance Coin", R.drawable.bnb));
                    put("BCH", new CoinNetworkModel("BCH", "Bitcoin Cash", R.drawable.bch));
                    put("DASH", new CoinNetworkModel("DASH", "", R.drawable.dash));
                    put("EOS", new CoinNetworkModel("EOS", "", R.drawable.eos));
                    put("NANO", new CoinNetworkModel("NANO", "", R.drawable.nano));
                    put("PAX", new CoinNetworkModel("PAX", "", R.drawable.pax));
                    put("TRX", new CoinNetworkModel("TRX", "", R.drawable.trx));
                    put("BUSD", new CoinNetworkModel("BUSD", "US Dollar", R.drawable.busd));
                    put("TUSD", new CoinNetworkModel("TUSD", "", R.drawable.tusd));
                    put("USDT", new CoinNetworkModel("USDT", "", R.drawable.usdt));
                }
            };

    private BinanceWidgetManager() {
        mSharedPreferences = ContextUtils.getAppSharedPreferences();
    }

    public static BinanceWidgetManager getInstance() {
        if (sInstance == null) {
            sInstance = new BinanceWidgetManager();
        }
        return sInstance;
    }

    public boolean isUserAuthenticatedForBinance() {
        return mSharedPreferences.getBoolean(PREF_BINANCE_USER_AUTHENTICATION, false);
    }

    public void setUserAuthenticationForBinance(boolean isAuthenticated) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_BINANCE_USER_AUTHENTICATION, isAuthenticated);
        sharedPreferencesEditor.apply();
    }

    public String getBinanceAccountBalance() {
        return mSharedPreferences.getString(PREF_BINANCE_ACCOUNT_BALANCE, "");
    }

    public void setBinanceAccountBalance(String accountBalance) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putString(PREF_BINANCE_ACCOUNT_BALANCE, accountBalance);
        sharedPreferencesEditor.apply();
    }
}