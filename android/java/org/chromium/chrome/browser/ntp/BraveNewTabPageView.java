/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import android.content.Context;
import android.content.SharedPreferences;
import android.util.AttributeSet;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import org.chromium.base.ContextUtils;
import org.chromium.base.TraceEvent;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ntp.NewTabPageView;

public class BraveNewTabPageView extends NewTabPageView {
    private static final String TAG = "BraveNewTabPageView";

    private static final String PREF_TRACKERS_BLOCKED_COUNT = "trackers_blocked_count";
    private static final String PREF_ADS_BLOCKED_COUNT = "ads_blocked_count";
    private static final String PREF_HTTPS_UPGRADES_COUNT = "https_upgrades_count";
    private static final short MILLISECONDS_PER_ITEM = 50;

    private TextView mAdsBlockedCountTextView;
    private TextView mHttpsUpgradesCountTextView;
    private TextView mEstTimeSavedTextView;
    private SharedPreferences mSharedPreferences;

    public BraveNewTabPageView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        ViewGroup braveStatsView = (ViewGroup) getNewTabPageLayout().findViewById(R.id.brave_stats);
        mAdsBlockedCountTextView = (TextView) braveStatsView.findViewById(R.id.brave_stats_text_ads_count);
        mHttpsUpgradesCountTextView = (TextView) braveStatsView.findViewById(R.id.brave_stats_text_https_count);
        mEstTimeSavedTextView = (TextView) braveStatsView.findViewById(R.id.brave_stats_text_time_count);
        mSharedPreferences = ContextUtils.getAppSharedPreferences();
    }

    @Override
    protected void onWindowVisibilityChanged(int visibility) {
        super.onWindowVisibilityChanged(visibility);
        if (visibility == VISIBLE) {
            updateBraveStats();
        }
    }

    /**
     * Sets up Brave stats.
     */
    private void updateBraveStats() {
        TraceEvent.begin(TAG + ".updateBraveStats()");

        long trackersBlockedCount = mSharedPreferences.getLong(PREF_TRACKERS_BLOCKED_COUNT, 0);
        long adsBlockedCount = mSharedPreferences.getLong(PREF_ADS_BLOCKED_COUNT, 0);
        long httpsUpgradesCount = mSharedPreferences.getLong(PREF_HTTPS_UPGRADES_COUNT, 0);
        long estimatedMillisecondsSaved = (trackersBlockedCount + adsBlockedCount) * MILLISECONDS_PER_ITEM;

        mAdsBlockedCountTextView.setText(getBraveStatsStringFormNumber(adsBlockedCount));
        mHttpsUpgradesCountTextView.setText(getBraveStatsStringFormNumber(httpsUpgradesCount));
        mEstTimeSavedTextView.setText(getBraveStatsStringFromTime(estimatedMillisecondsSaved / 1000));
        TraceEvent.end(TAG + ".updateBraveStats()");
    }

    /*
    * Gets string view of specific number for Brave stats
    */
    private String getBraveStatsStringFormNumber(long number) {
        String result = "";
        String suffix = "";
        if (number >= 1000 * 1000 * 1000) {
            result = result + (number / (1000 * 1000 * 1000));
            number = number % (1000 * 1000 * 1000);
            result = result + "." + (number / (10 * 1000 * 1000));
            suffix = "B";
        }
        else if (number >= (10 * 1000 * 1000) && number < (1000 * 1000 * 1000)) {
            result = result + (number / (1000 * 1000));
            suffix = "M";
        }
        else if (number >= (1000 * 1000) && number < (10 * 1000 * 1000)) {
            result = result + (number / (1000 * 1000));
            number = number % (1000 * 1000);
            result = result + "." + (number / (100 * 1000));
            suffix = "M";
        }
        else if (number >= (10 * 1000) && number < (1000 * 1000)) {
            result = result + (number / 1000);
            suffix = "K";
        }
        else if (number >= 1000 && number < (10* 1000)) {
            result = result + (number / 1000);
            number = number % 1000;
            result = result + "." + (number / 100);
            suffix = "K";
        }
        else {
            result = result + number;
        }
        result = result + suffix;
        return result;
    }

    /*
    * Gets string view of specific time in seconds for Brave stats
    */
    private String getBraveStatsStringFromTime(long seconds) {
        String result = "";
        if (seconds > 24 * 60 * 60) {
            result = result + (seconds / (24 * 60 * 60)) + "d";
        }
        else if (seconds > 60 * 60) {
            result = result + (seconds / (60 * 60)) + "h";
        }
        else if (seconds > 60) {
            result = result + (seconds / 60) + "m";
        }
        else {
            result = result + seconds + "s";
        }
        return result;
    }
}
