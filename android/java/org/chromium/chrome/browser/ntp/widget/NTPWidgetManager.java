/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.ntp.widget;

import android.content.SharedPreferences;
import android.content.Context;

import org.chromium.chrome.R;
import org.chromium.base.ContextUtils;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.HashMap;
import java.util.TreeMap;

import org.chromium.chrome.browser.ntp.widget.NTPWidgetItem;

public class NTPWidgetManager {
    public static final String PREF_PRIVATE_STATS = "private_stats";
    public static final String PREF_FAVORITES = "favorites";
    // public static final String PREF_BRAVE_REWARDS = "brave_rewards";
    public static final String PREF_BINANCE = "binance";
    public static final String PREF_NTP_WIDGET_ORDER = "ntp_widget_order";

    private static NTPWidgetManager sInstance;

    private final SharedPreferences mSharedPreferences;
    private static Context mContext = ContextUtils.getApplicationContext();

    public static Map<String, NTPWidgetItem> mWidgetsMap = new HashMap<String, NTPWidgetItem>() {
        {
            put(PREF_PRIVATE_STATS,
                    new NTPWidgetItem(PREF_PRIVATE_STATS,
                        mContext.getResources().getString(R.string.privacy_stats),
                        mContext.getResources().getString(R.string.privacy_stats_text)));
            put(PREF_FAVORITES,
                    new NTPWidgetItem(PREF_FAVORITES,
                        mContext.getResources().getString(R.string.favorites),
                        mContext.getResources().getString(R.string.privacy_stats_text)));
            put(PREF_BINANCE,
                    new NTPWidgetItem(PREF_BINANCE,
                        mContext.getResources().getString(R.string.binance),
                        mContext.getResources().getString(R.string.privacy_stats_text)));
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

    // public int getBraveRewardsWidget() {
    //     return mSharedPreferences.getInt(PREF_BRAVE_REWARDS, 2);
    // }

    public int getBinanceWidget() {
        return mSharedPreferences.getInt(PREF_BINANCE, 2);
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
        if (getBinanceWidget() != -1) {
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
        if (getBinanceWidget() == -1) {
            availableWidgets.add(PREF_BINANCE);
        }
        return availableWidgets;
    }
}