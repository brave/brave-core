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
import android.content.pm.ActivityInfo;

import com.google.android.material.bottomsheet.BottomSheetDialogFragment;
import com.google.android.material.bottomsheet.BottomSheetDialog;
import com.google.android.material.bottomsheet.BottomSheetBehavior;
import android.view.ViewTreeObserver;

import org.chromium.chrome.R;

import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.ui.base.DeviceFormFactor;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
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
    private static final short MILLISECONDS_PER_ITEM = 50;
    private DatabaseHelper mDatabaseHelper = DatabaseHelper.getInstance();

    private static final int WEBSITES = 0;
    private static final int TRACKERS = 1;

    private static final int DAYS_7 = -7;
    private static final int DAYS_30 = -30;
    private static final int DAYS_90 = -90;


    private TextView adsTrackersCountText;
    private TextView dataSavedCountText;
    private TextView dataSavedText;
    private TextView timeSavedCountText;
    private TextView timeSavedText;
    private TextView noDataText;
    private LinearLayout websitesLayout;
    private LinearLayout trackersLayout;

    private int selectedType = WEBSITES;
    private int selectedDuration = DAYS_7;

    private Context mContext;

    public static BraveStatsBottomSheetDialogFragment newInstance() {
        return new BraveStatsBottomSheetDialogFragment();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mContext = ContextUtils.getApplicationContext();
        getActivity().setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
    }

    @Nullable
    @Override
    public View onCreateView(LayoutInflater inflater,
                             @Nullable ViewGroup container,
                             @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.brave_stats_bottom_sheet, container, false);
    }

    @Override
    public void onDestroyView() {
        getActivity().setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR);
        super.onDestroyView();
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        RadioGroup durationRadioGroup = view.findViewById(R.id.duration_radio_group);
        durationRadioGroup.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup radioGroup, int checkedId) {
                if (checkedId == R.id.week_radio) {
                    selectedDuration = DAYS_7;
                } else if (checkedId == R.id.month_radio) {
                    selectedDuration = DAYS_30;
                } else if (checkedId == R.id.months_radio) {
                    selectedDuration = DAYS_90;
                }
                updateBraveStatsLayout();
            }
        });

        LinearLayout layout = view.findViewById(R.id.brave_stats_layout);
        adsTrackersCountText = layout.findViewById(R.id.ads_trackers_count_text);
        dataSavedCountText = layout.findViewById(R.id.data_saved_count_text);
        dataSavedText = layout.findViewById(R.id.data_saved_text);
        timeSavedCountText = layout.findViewById(R.id.time_saved_count_text);
        timeSavedText = layout.findViewById(R.id.time_saved_text);
        websitesLayout = layout.findViewById(R.id.wesites_layout);
        trackersLayout = layout.findViewById(R.id.trackers_layout);

        RadioGroup statTypeRadioGroup = layout.findViewById(R.id.stat_type_radio_group);
        statTypeRadioGroup.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup radioGroup, int checkedId) {
                if (checkedId == R.id.websites_radio) {
                    selectedType = WEBSITES;
                } else if (checkedId == R.id.trackers_radio) {
                    selectedType = TRACKERS;
                }
                showWebsitesTrackers();
            }
        });

        noDataText = layout.findViewById(R.id.empty_data_text);
        ImageView btnClose = view.findViewById(R.id.brave_stats_bottom_sheet_close);
        btnClose.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                dismiss();
            }
        });
        updateBraveStatsLayout();
    }

    private void updateBraveStatsLayout() {
        long totalSavedBandwidth = mDatabaseHelper.getTotalSavedBandwidthWithDate(BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", selectedDuration), BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", 0));

        long adsTrackersCount = mDatabaseHelper.getAllStatsWithDate(BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", selectedDuration), BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", 0)).size();
        long timeSavedCount = adsTrackersCount * MILLISECONDS_PER_ITEM;

        Pair<String, String> adsTrackersPair = BraveStatsUtil.getBraveStatsStringFormNumberPair(adsTrackersCount, false);
        adsTrackersCountText.setText(String.format(getResources().getString(R.string.ntp_stat_text), adsTrackersPair.first, adsTrackersPair.second));

        Pair<String, String> dataSavedPair = BraveStatsUtil.getBraveStatsStringFormNumberPair(totalSavedBandwidth, true);
        dataSavedCountText.setText(dataSavedPair.first);
        dataSavedText.setText(String.format(mContext.getResources().getString(R.string.data_saved_text), dataSavedPair.second));

        timeSavedCountText.setText(BraveStatsUtil.getBraveStatsStringFromTime(timeSavedCount / 1000));
        timeSavedText.setText(mContext.getResources().getString(R.string.time_saved_text));

        showWebsitesTrackers();
    }

    private void showWebsitesTrackers() {
        List<Pair<String, Integer>> websiteTrackers = null;
        LinearLayout rootView = null;
        if (selectedType == WEBSITES) {
            websiteTrackers = mDatabaseHelper.getStatsWithDate(BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", selectedDuration), BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", 0));
            websitesLayout.setVisibility(View.VISIBLE);
            trackersLayout.setVisibility(View.GONE);
            rootView = websitesLayout;
        } else {
            websiteTrackers = mDatabaseHelper.getSitesWithDate(BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", selectedDuration), BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", 0));
            websitesLayout.setVisibility(View.GONE);
            trackersLayout.setVisibility(View.VISIBLE);
            rootView = trackersLayout;
        }

        rootView.removeAllViews();

        if (websiteTrackers.size() > 0) {
            for (Pair<String, Integer> statPair : websiteTrackers) {
                LayoutInflater inflater = LayoutInflater.from(mContext);
                ViewGroup layout = (ViewGroup) inflater.inflate(R.layout.tracker_item_layout, null);

                TextView mTrackerCountText = (TextView) layout.findViewById(R.id.tracker_count_text);
                TextView mSiteText = (TextView) layout.findViewById(R.id.site_text);

                mTrackerCountText.setText(String.valueOf(statPair.second));
                mTrackerCountText.setTextColor(getResources().getColor(R.color.brave_stats_text_color));
                mSiteText.setText(statPair.first);
                mSiteText.setTextColor(getResources().getColor(R.color.brave_stats_text_color));

                rootView.addView(layout);
            }
            noDataText.setVisibility(View.GONE);
        } else {
            noDataText.setVisibility(View.VISIBLE);
        }
    }
}
