/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_stats;

import android.os.Bundle;
import android.content.Context;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import android.widget.LinearLayout;
import com.google.android.material.tabs.TabLayout;
import androidx.viewpager.widget.PagerAdapter;
import androidx.viewpager.widget.ViewPager;
import android.widget.ImageView;
import android.widget.FrameLayout;
import android.content.res.Configuration;
import android.widget.RadioGroup;
import android.util.Pair;

import com.google.android.material.bottomsheet.BottomSheetDialogFragment;
import com.google.android.material.bottomsheet.BottomSheetDialog;
import com.google.android.material.bottomsheet.BottomSheetBehavior;
import android.view.ViewTreeObserver;

import org.chromium.chrome.R;

import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.ui.base.DeviceFormFactor;
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

import static org.chromium.ui.base.ViewUtils.dpToPx;

public class BraveStatsBottomSheetDialogFragment extends BottomSheetDialogFragment {
    private static final Context mContext = ContextUtils.getApplicationContext();
    private static final short MILLISECONDS_PER_ITEM = 50;
    private DatabaseHelper mDatabaseHelper = DatabaseHelper.getInstance();

    private static final int DAYS_7 = -7;
    private static final int DAYS_30 = -30;
    private static final int DAYS_90 = -90;

    private TextView noDataText;

    public static BraveStatsBottomSheetDialogFragment newInstance() {
        return new BraveStatsBottomSheetDialogFragment();
    }

    @Nullable
    @Override
    public View onCreateView(LayoutInflater inflater,
                             @Nullable ViewGroup container,
                             @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.brave_stats_bottom_sheet, container, false);
    }

    @Override
    public void onPause() {
        super.onPause();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(getActivity());
        if (isTablet || (!isTablet && newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE)) {
            getDialog().getWindow().setLayout(dpToPx(getActivity(), 400), -1);
        } else {
            getDialog().getWindow().setLayout(-1, -1);
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(getActivity());
        if (isTablet || (!isTablet && ConfigurationUtils.isLandscape(getActivity()))) {
            getDialog().getWindow().setLayout(dpToPx(getActivity(), 400), -1);
        } else {
            getDialog().getWindow().setLayout(-1, -1);
        }
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        LinearLayout layout = view.findViewById(R.id.brave_stats_layout);

        int datePeriod = DAYS_7;
        // switch (position) {
        // case 0:
        //     datePeriod = DAYS_7;
        //     break;
        // case 1:
        //     datePeriod = DAYS_30;
        //     break;
        // case 2:
        //     datePeriod = DAYS_90;
        //     break;
        // }

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

        ImageView btnClose = view.findViewById(R.id.brave_stats_bottom_sheet_close);
        btnClose.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                dismiss();
            }
        });
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

}
