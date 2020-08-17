/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import android.widget.Button;
import android.util.Pair;

import androidx.viewpager.widget.PagerAdapter;
import androidx.viewpager.widget.ViewPager;

import org.chromium.chrome.R;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.ntp_background_images.util.NTPUtil;
import org.chromium.chrome.browser.brave_stats.BraveStatsUtil;
import org.chromium.chrome.browser.widget.crypto.binance.CryptoWidgetBottomSheetDialogFragment;

import java.util.ArrayList;

public class NTPWidgetAdapter extends PagerAdapter {

    private static final String PREF_TRACKERS_BLOCKED_COUNT = "trackers_blocked_count";
    private static final String PREF_ADS_BLOCKED_COUNT = "ads_blocked_count";
    private static final String PREF_HTTPS_UPGRADES_COUNT = "https_upgrades_count";

    private ArrayList<View> views = new ArrayList<View>();
    private Context context = ContextUtils.getApplicationContext();
    private Profile profile = Profile.getLastUsedRegularProfile();

    @Override
    public int getItemPosition (Object object) {
        int index = views.indexOf (object);
        if (index == -1)
            return POSITION_NONE;
        else
            return index;
    }

    @Override
    public Object instantiateItem (ViewGroup container, int position) {
        View v = views.get (position);

        if (0 == position) {
            updateBraveStats(v);
        } else if (1 == position) {
            Button btnTrade = v.findViewById(R.id.btn_trade);
            btnTrade.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    // CryptoWidgetBottomSheetDialogFragment cryptoWidgetBottomSheetDialogFragment = new CryptoWidgetBottomSheetDialogFragment();
                    // cryptoWidgetBottomSheetDialogFragment.show(getSupportFragmentManager(), "brave_stats_bottom_sheet_dialog_fragment");
                }
            });
        }

        container.addView (v);
        return v;
    }

    @Override
    public void destroyItem (ViewGroup container, int position, Object object) {
        container.removeView (views.get (position));
    }

    @Override
    public int getCount () {
        return views.size();
    }

    @Override
    public boolean isViewFromObject (View view, Object object) {
        return view == object;
    }

    public int addView (View v) {
        return addView (v, views.size());
    }

    public int addView (View v, int position) {
        views.add (position, v);
        return position;
    }

    public int removeView (ViewPager pager, View v) {
        return removeView (pager, views.indexOf (v));
    }

    public int removeView (ViewPager pager, int position) {
        pager.setAdapter (null);
        views.remove (position);
        pager.setAdapter (this);

        return position;
    }

    public View getView (int position) {
        return views.get (position);
    }

    private void updateBraveStats(View view) {
        ViewGroup mBraveStatsView = view.findViewById(R.id.brave_stats_layout);

        TextView mAdsBlockedCountTextView = (TextView) mBraveStatsView.findViewById(R.id.brave_stats_text_ads_count);
        TextView mDataSavedValueTextView = (TextView) mBraveStatsView.findViewById(R.id.brave_stats_data_saved_value);
        TextView mEstTimeSavedCountTextView = (TextView) mBraveStatsView.findViewById(R.id.brave_stats_text_time_count);

        TextView mAdsBlockedTextView = (TextView) mBraveStatsView.findViewById(R.id.brave_stats_text_ads);
        TextView mDataSavedTextView = (TextView) mBraveStatsView.findViewById(R.id.brave_stats_data_saved_text);
        TextView mEstTimeSavedTextView = (TextView) mBraveStatsView.findViewById(R.id.brave_stats_text_time);

        long trackersBlockedCount = BravePrefServiceBridge.getInstance().getTrackersBlockedCount(profile);
        long adsBlockedCount = BravePrefServiceBridge.getInstance().getAdsBlockedCount(profile);
        long dataSaved = BravePrefServiceBridge.getInstance().getDataSaved(profile);
        long estimatedMillisecondsSaved = (trackersBlockedCount + adsBlockedCount) * BraveStatsUtil.MILLISECONDS_PER_ITEM;

        Pair<String, String> adsTrackersPair = BraveStatsUtil.getBraveStatsStringFormNumberPair(adsBlockedCount, false);
        Pair<String, String> dataSavedPair = BraveStatsUtil.getBraveStatsStringFormNumberPair(dataSaved, true);

        mAdsBlockedCountTextView.setText(String.format(context.getResources().getString(R.string.ntp_stat_text), adsTrackersPair.first, adsTrackersPair.second));
        mDataSavedValueTextView.setText(String.format(context.getResources().getString(R.string.ntp_stat_text), dataSavedPair.first, dataSavedPair.second));
        mEstTimeSavedCountTextView.setText(BraveStatsUtil.getBraveStatsStringFromTime(estimatedMillisecondsSaved / 1000));

        if ((BravePrefServiceBridge.getInstance().getBoolean(BravePref.NTP_SHOW_BACKGROUND_IMAGE)
                || NTPUtil.isReferralEnabled())
                && NTPUtil.shouldEnableNTPFeature()) {
            mAdsBlockedTextView.setTextColor(
                context.getResources().getColor(android.R.color.white));
            mDataSavedTextView.setTextColor(
                context.getResources().getColor(android.R.color.white));
            mEstTimeSavedTextView.setTextColor(
                context.getResources().getColor(android.R.color.white));
            mEstTimeSavedCountTextView.setTextColor(
                context.getResources().getColor(android.R.color.white));
        }
    }
}