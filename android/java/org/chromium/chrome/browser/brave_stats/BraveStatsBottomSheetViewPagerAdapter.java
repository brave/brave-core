/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.brave_stats;

import android.content.Context;
import androidx.viewpager.widget.PagerAdapter;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import android.widget.LinearLayout;
import android.widget.Button;
import android.widget.ImageView;
import android.view.LayoutInflater;
import com.google.android.material.tabs.TabLayout;
import androidx.viewpager.widget.ViewPager;
import com.airbnb.lottie.LottieAnimationView;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.LinearLayoutManager;
import android.widget.RadioGroup;
import android.util.Pair;

import org.chromium.chrome.R;

import org.chromium.base.Log;
import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;
import org.chromium.chrome.browser.local_database.DatabaseHelper;
import org.chromium.chrome.browser.local_database.BraveStatsTable;
import org.chromium.chrome.browser.local_database.SavedBandwidthTable;
import org.chromium.chrome.browser.brave_stats.BraveStatsUtil;

import java.util.List;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Date;

public class BraveStatsBottomSheetViewPagerAdapter extends PagerAdapter {
    private static final Context mContext = ContextUtils.getApplicationContext();
    private static final short MILLISECONDS_PER_ITEM = 50;
    private DatabaseHelper mDatabaseHelper = DatabaseHelper.getInstance();

    private static final int DAYS_7 = -7;
    private static final int DAYS_30 = -30;
    private static final int DAYS_90 = -90;

    private TextView noDataText;

    private static final List<String> mHeaders = Arrays.asList(
                mContext.getResources().getString(R.string.week),
                mContext.getResources().getString(R.string.month),
                mContext.getResources().getString(R.string.months_3)
            );

    public BraveStatsBottomSheetViewPagerAdapter() {

    }

    @Override
    public int getCount() {
        return mHeaders.size();
    }

    @Override
    public CharSequence getPageTitle(int position) {
        // Generate title based on item position
        return mHeaders.get(position);
    }

    @Override
    public boolean isViewFromObject(View view, Object viewObject) {
        return viewObject == view;
    }

    @Override
    public Object instantiateItem(final ViewGroup container, int position) {
        LayoutInflater inflater = LayoutInflater.from(mContext);
        ViewGroup layout = (ViewGroup) inflater.inflate(R.layout.brave_stats_pager_layout,
                           container, false);

        int datePeriod = DAYS_7;
        switch (position) {
        case 0:
            datePeriod = DAYS_7;
            break;
        case 1:
            datePeriod = DAYS_30;
            break;
        case 2:
            datePeriod = DAYS_90;
            break;
        }

        noDataText = layout.findViewById(R.id.empty_data_text);

        long adsTrackersCount = mDatabaseHelper.getAllStats().size();
        long timeSavedCount = adsTrackersCount * MILLISECONDS_PER_ITEM;

        TextView adsTrackersCountText = layout.findViewById(R.id.ads_trackers_count_text);
        Pair<String, String> adsTrackersPair = BraveStatsUtil.getBraveStatsStringFormNumberPair(adsTrackersCount, false);
        adsTrackersCountText.setText(adsTrackersPair.first + adsTrackersPair.second);

        TextView dataSavedCountText = layout.findViewById(R.id.data_saved_count_text);
        Pair<String, String> dataSavedPair = BraveStatsUtil.getBraveStatsStringFormNumberPair(mDatabaseHelper.getTotalSavedBandwidth(), true);
        dataSavedCountText.setText(dataSavedPair.first);

        TextView dataSavedText = layout.findViewById(R.id.data_saved_text);
        dataSavedText.setText(String.format(mContext.getResources().getString(R.string.data_saved_text), dataSavedPair.second));

        TextView timeSavedCountText = layout.findViewById(R.id.time_saved_count_text);
        Pair<String, String> timeSavedPair = BraveStatsUtil.getBraveStatsStringFromTimePair(timeSavedCount / 1000);
        timeSavedCountText.setText(timeSavedPair.first);

        TextView timeSavedText = layout.findViewById(R.id.time_saved_text);
        timeSavedText.setText(String.format(mContext.getResources().getString(R.string.time_saved_text), timeSavedPair.second));

        final LinearLayout websitesLayout = layout.findViewById(R.id.wesites_layout);
        showWebsitesTrackers(websitesLayout, getWebsitesTrackersForDate(datePeriod, true), true);

        final LinearLayout trackersLayout = layout.findViewById(R.id.trackers_layout);
        showWebsitesTrackers(trackersLayout, getWebsitesTrackersForDate(datePeriod, false), false);

        websitesLayout.setVisibility(View.VISIBLE);
        trackersLayout.setVisibility(View.GONE);

        RadioGroup radioGroup = layout.findViewById(R.id.radio_group);
        radioGroup.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup radioGroup, int checkedId) {
                if (checkedId == R.id.websites_radio) {
                    websitesLayout.setVisibility(View.VISIBLE);
                    trackersLayout.setVisibility(View.GONE);
                } else if (checkedId == R.id.trackers_radio) {
                    websitesLayout.setVisibility(View.GONE);
                    trackersLayout.setVisibility(View.VISIBLE);
                }
            }
        });

        container.addView(layout);
        return layout;
    }

    private List<Pair<String, Integer>> getWebsitesTrackersForDate(int datePeriod, boolean websitesShown) {
        if (websitesShown) {
            return mDatabaseHelper.getStatsWithDate(datePeriod);
        } else {
            return mDatabaseHelper.getSitesWithDate();
        }
    }

    private void showWebsitesTrackers(LinearLayout webSitesTrackersLayout, List<Pair<String, Integer>> websiteTrackers, boolean websitesShown) {
        if (websiteTrackers.size() > 0) {
            for (Pair<String, Integer> statPair : websiteTrackers) {
                LayoutInflater inflater = LayoutInflater.from(mContext);
                ViewGroup layout = (ViewGroup) inflater.inflate(R.layout.tracker_item_layout, null);

                TextView mTrackerCountText = (TextView) layout.findViewById(R.id.tracker_count_text);
                TextView mSiteText = (TextView) layout.findViewById(R.id.site_text);

                mTrackerCountText.setVisibility(View.VISIBLE);
                mTrackerCountText.setText(String.valueOf(statPair.second));

                mSiteText.setText(statPair.first);

                webSitesTrackersLayout.addView(layout);
            }
            noDataText.setVisibility(View.GONE);
        } else {
            noDataText.setVisibility(View.VISIBLE);
        }
    }

    @Override
    public void destroyItem(ViewGroup container, int position, Object object) {
        container.removeView((View)object);
    }
}
