/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.ntp.widget;

import android.content.Context;
import android.content.SharedPreferences;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ntp.widget.NTPWidgetItem;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceNativeWorker;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

public class NTPWidgetManager {
    public static final String PREF_PRIVATE_STATS = "private_stats";
    public static final String PREF_FAVORITES = "favorites";
    public static final String PREF_BINANCE = "binance";
    public static final String PREF_NTP_WIDGET_ORDER = "ntp_widget_order";

    private static NTPWidgetManager sInstance;

    private final SharedPreferences mSharedPreferences;

    public static Map<String, NTPWidgetItem> mWidgetsMap = new HashMap<String, NTPWidgetItem>() {
        {
            put(PREF_PRIVATE_STATS,
                    new NTPWidgetItem(PREF_PRIVATE_STATS,
                            ContextUtils.getApplicationContext().getResources().getString(
                                    R.string.privacy_stats),
                            ContextUtils.getApplicationContext().getResources().getString(
                                    R.string.privacy_stats_text)));
            put(PREF_FAVORITES,
                    new NTPWidgetItem(PREF_FAVORITES,
                            ContextUtils.getApplicationContext().getResources().getString(
                                    R.string.favorites),
                            ContextUtils.getApplicationContext().getResources().getString(
                                    R.string.favorites_text)));
            put(PREF_BINANCE,
                    new NTPWidgetItem(PREF_BINANCE,
                            ContextUtils.getApplicationContext().getResources().getString(
                                    R.string.binance),
                            ContextUtils.getApplicationContext().getResources().getString(
                                    R.string.binance_disconnect_text)));
        }
    };

    private NTPWidgetManager() {
        mSharedPreferences = ContextUtils.getAppSharedPreferences();
    }

    public static NTPWidgetManager getInstance() {
        if (sInstance == null) {
            sInstance = new NTPWidgetManager();
        }
        return sInstance;
    }

    public int getPrivateStatsWidget() {
        return mSharedPreferences.getInt(PREF_PRIVATE_STATS, 0);
    }

    public int getFavoritesWidget() {
        return mSharedPreferences.getInt(PREF_FAVORITES, 1);
    }

    public int getBinanceWidget() {
        return mSharedPreferences.getInt(PREF_BINANCE, -1);
    }

    public void setWidget(String widgetType, int position) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putInt(widgetType, position);
        sharedPreferencesEditor.apply();
    }

    public int getNTPWidgetOrder() {
        return mSharedPreferences.getInt(PREF_NTP_WIDGET_ORDER, 0);
    }

    public void setNTPWidgetOrder(int position) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putInt(PREF_NTP_WIDGET_ORDER, position);
        sharedPreferencesEditor.apply();
    }

    public List<String> getUsedWidgets() {
        Map<Integer, String> usedWidgetMap = new TreeMap<>();
        if (getPrivateStatsWidget() != -1) {
            usedWidgetMap.put(getPrivateStatsWidget(), PREF_PRIVATE_STATS);
        }
        if (getFavoritesWidget() != -1) {
            usedWidgetMap.put(getFavoritesWidget(), PREF_FAVORITES);
        }
        if (getBinanceWidget() != -1
            && BinanceNativeWorker.getInstance().IsSupportedRegion()) {
            usedWidgetMap.put(getBinanceWidget(), PREF_BINANCE);
        }
        return new ArrayList<String>(usedWidgetMap.values());
    }

    public List<String> getAvailableWidgets() {
        List<String> availableWidgets = new ArrayList<>();
        if (getPrivateStatsWidget() == -1) {
            availableWidgets.add(PREF_PRIVATE_STATS);
        }
        if (getFavoritesWidget() == -1) {
            availableWidgets.add(PREF_FAVORITES);
        }
        // if (getBinanceWidget() == -1
        //     && BinanceNativeWorker.getInstance().IsSupportedRegion()) {
        //     availableWidgets.add(PREF_BINANCE);
        // }
        return availableWidgets;
    }
}