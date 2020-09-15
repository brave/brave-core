/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.widget.crypto.binance;

import android.content.SharedPreferences;

import org.chromium.base.ContextUtils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;
import java.util.HashMap;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceAccountBalance;
import org.chromium.chrome.browser.widget.crypto.binance.CoinNetworkModel;

public class BinanceWidgetManager {
    public static final String PREF_BINANCE_USER_AUTHENTICATION = "binance_user_authentication";

    public static final String BINANCE_US = "https://www.binance.us/en/buy-sell-crypto?crypto=%1$s&amount=%2$s&ref=35089877&utm_source=brave";
    public static final String BINANCE_COM = "https://www.binance.com/en/buy-sell-crypto?fiat=%1$s&crypto=%2$s&amount=%3$s&ref=39346846&utm_source=brave";

    private static BinanceWidgetManager sInstance;

    private final SharedPreferences mSharedPreferences;

    private static BinanceAccountBalance binanceAccountBalance;

    public static final List<String> fiatList = Arrays.asList("USD","AUD","ARS","BGN","BRL","CAD","CHF","CNY","COP","CZK","DKK","EUR","GBP","HKD","HRK","HUF","IDR","ILS","INR","JPY","KRW","KZT","MXN","NGN","NOK","NZD","PLN","RON","RUB","SEK","THB","TRY","UAH","VND","ZAR");
    public static final Map<String, CoinNetworkModel> usCurrenciesMap = new HashMap<String, CoinNetworkModel>() {
        {
            put("BTC", new CoinNetworkModel("BTC", "Bitcoin", R.drawable.btc));
            put("ETH", new CoinNetworkModel("ETH", "Ethereum", R.drawable.eth));
            put("BNB", new CoinNetworkModel("BNB", "Binance Coin", R.drawable.bnb));
            put("BCH", new CoinNetworkModel("BCH", "Bitcoin Cash", R.drawable.bch));
            put("XRP", new CoinNetworkModel("XRP", "Ripple", R.drawable.xrp));
            put("BUSD", new CoinNetworkModel("BUSD", "US Dollar", R.drawable.eth));
            put("BAT", new CoinNetworkModel("BAT", "Basic Attention Token", R.drawable.bat));
            put("XTZ", new CoinNetworkModel("XTZ", "", R.drawable.xtz));
            put("ALGO", new CoinNetworkModel("ALGO", "", R.drawable.eth));
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
            put("HBAR", new CoinNetworkModel("HBAR", "", R.drawable.eth));
            put("MATIC", new CoinNetworkModel("MATIC", "", R.drawable.matic));
        }
    };
    public static final Map<String, CoinNetworkModel> comCurrenciesMap = new HashMap<String, CoinNetworkModel>() {
        {
            put("BTC", new CoinNetworkModel("BTC", "Bitcoin", R.drawable.btc));
            put("ETH", new CoinNetworkModel("ETH", "Ethereum", R.drawable.eth));
            put("BAT", new CoinNetworkModel("BAT", "Basic Attention Token", R.drawable.bat));
            put("XRP", new CoinNetworkModel("XRP", "Ripple", R.drawable.xrp));
            put("BNB", new CoinNetworkModel("BNB", "Binance Coin", R.drawable.bnb));
            put("BCH", new CoinNetworkModel("BCH", "Bitcoin Cash", R.drawable.bch));
            put("DASH", new CoinNetworkModel("DASH", "", R.drawable.dash));
            put("EOS", new CoinNetworkModel("EOS", "", R.drawable.eos));
            put("NANO", new CoinNetworkModel("NANO", "", R.drawable.nano));
            put("PAX", new CoinNetworkModel("PAX", "", R.drawable.pax));
            put("TRX", new CoinNetworkModel("TRX", "", R.drawable.trx));
            put("BUSD", new CoinNetworkModel("BUSD", "US Dollar", R.drawable.eth));
            put("TUSD", new CoinNetworkModel("TUSD", "", R.drawable.tusd));
            put("USDT", new CoinNetworkModel("USDT", "", R.drawable.usdt));
        }
    };

    public BinanceAccountBalance getBinanceAccountBalance() {
        return binanceAccountBalance;
    }

    public void setBinanceAccountBalance(BinanceAccountBalance binanceAccountBalance) {
        this.binanceAccountBalance = binanceAccountBalance;
    }

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
}