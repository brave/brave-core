/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.ntp.widget;

import android.content.SharedPreferences;

import org.chromium.base.ContextUtils;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

public class NTPWidgetManager {
    public static final String PREF_PRIVATE_STATS = "private_stats";
    public static final String PREF_FAVORITES = "favorites";
    public static final String PREF_BRAVE_REWARDS = "brave_rewards";
    public static final String PREF_BINANCE = "binance";

    private static NTPWidgetManager sInstance;

    private final SharedPreferences mSharedPreferences;

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

    public int getBraveRewardsWidget() {
        return mSharedPreferences.getInt(PREF_BRAVE_REWARDS, 2);
    }

    public int getBinanceWidget() {
        return mSharedPreferences.getInt(PREF_BINANCE, 3);
    }

    public void setWidget(String widgetType, int position) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putInt(widgetType, position);
        sharedPreferencesEditor.apply();
    }

    public List<NTPWidgetItem> getWidgetList() {
        Map<Integer, NTPWidgetItem> ntpWidgetMap = new TreeMap<>();
        if (getPrivateStatsWidget() != -1) {
            ntpWidgetMap.put(getPrivateStatsWidget(),
                    new NTPWidgetItem(PREF_PRIVATE_STATS, "Privacy Stats",
                            "Trackers &amp; Ads Blocked, Saved Bandwidth, and Time Saved Estimates."));
        }
        if (getFavoritesWidget() != -1) {
            ntpWidgetMap.put(getFavoritesWidget(),
                    new NTPWidgetItem(PREF_FAVORITES, "Favorites",
                            "Trackers &amp; Ads Blocked, Saved Bandwidth, and Time Saved Estimates."));
        }
        if (getBraveRewardsWidget() != -1) {
            ntpWidgetMap.put(getBraveRewardsWidget(),
                    new NTPWidgetItem(PREF_BRAVE_REWARDS, "Brave Rewards",
                            "Trackers &amp; Ads Blocked, Saved Bandwidth, and Time Saved Estimates."));
        }
        if (getBinanceWidget() != -1) {
            ntpWidgetMap.put(getBinanceWidget(),
                    new NTPWidgetItem(PREF_BINANCE, "Binance",
                            "Trackers &amp; Ads Blocked, Saved Bandwidth, and Time Saved Estimates."));
        }

        return new ArrayList<NTPWidgetItem>(ntpWidgetMap.values());
    }
}