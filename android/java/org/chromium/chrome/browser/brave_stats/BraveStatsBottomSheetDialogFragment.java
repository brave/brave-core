/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_stats;

import static org.chromium.ui.base.ViewUtils.dpToPx;

import android.annotation.SuppressLint;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.util.DisplayMetrics;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.view.ViewTreeObserver;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;

import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;
import androidx.viewpager.widget.PagerAdapter;
import androidx.viewpager.widget.ViewPager;

import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.google.android.material.bottomsheet.BottomSheetDialog;
import com.google.android.material.bottomsheet.BottomSheetDialogFragment;
import com.google.android.material.tabs.TabLayout;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.task.AsyncTask;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.brave_stats.BraveStatsUtil;
import org.chromium.chrome.browser.local_database.BraveStatsTable;
import org.chromium.chrome.browser.local_database.DatabaseHelper;
import org.chromium.chrome.browser.local_database.SavedBandwidthTable;
import org.chromium.chrome.browser.night_mode.GlobalNightModeStateProviderHolder;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;
import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.ui.base.DeviceFormFactor;

import java.util.Arrays;
import java.util.Calendar;
import java.util.Date;
import java.util.List;

public class BraveStatsBottomSheetDialogFragment extends BottomSheetDialogFragment {
    final public static String TAG_FRAGMENT = "BRAVESTATS_FRAG";
    private DatabaseHelper mDatabaseHelper = DatabaseHelper.getInstance();

    private static final int WEBSITES = 0;
    private static final int TRACKERS = 1;

    private static final int DAYS_7 = -7;
    private static final int DAYS_30 = -30;
    private static final int DAYS_90 = -90;

    private TextView adsTrackersCountText;
    private TextView adsTrackersText;
    private TextView dataSavedCountText;
    private TextView dataSavedText;
    private TextView timeSavedCountText;
    private TextView timeSavedText;
    private TextView noDataText;
    private TextView braveStatsSubSectionText;
    private LinearLayout emptyDataLayout;
    private LinearLayout websitesLayout;
    private LinearLayout trackersLayout;

    private RadioButton monthRadioButton;
    private RadioButton monthsRadioButton;

    private int selectedType = WEBSITES;
    private int selectedDuration = DAYS_7;

    private Context mContext;

    public static BraveStatsBottomSheetDialogFragment newInstance() {
        return new BraveStatsBottomSheetDialogFragment();
    }

    @Override
    @SuppressLint("SourceLockedOrientationActivity")
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mContext = ContextUtils.getApplicationContext();
        getActivity().setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
    }

    @Override
    public void show(FragmentManager manager, String tag) {
        try {
            BraveStatsBottomSheetDialogFragment fragment = (BraveStatsBottomSheetDialogFragment) manager.findFragmentByTag(BraveStatsBottomSheetDialogFragment.TAG_FRAGMENT);
            FragmentTransaction transaction = manager.beginTransaction();
            if (fragment != null) {
                transaction.remove(fragment);
            }
            transaction.add(this, tag);
            transaction.commitAllowingStateLoss();
        } catch (IllegalStateException e) {
            Log.e("BraveStatsBottomSheetDialogFragment", e.getMessage());
        }
    }

    @Override
    public void setupDialog(Dialog dialog, int style) {
        super.setupDialog(dialog, style);

        final View view = LayoutInflater.from(getContext()).inflate(R.layout.brave_stats_bottom_sheet, null);

        emptyDataLayout = view.findViewById(R.id.brave_stats_empty_layout);

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
                updateBraveStatsLayoutAsync();
            }
        });

        monthRadioButton = view.findViewById(R.id.month_radio);
        monthsRadioButton = view.findViewById(R.id.months_radio);

        LinearLayout layout = view.findViewById(R.id.brave_stats_layout);
        adsTrackersCountText = layout.findViewById(R.id.ads_trackers_count_text);
        adsTrackersText = layout.findViewById(R.id.ads_trackers_text);
        dataSavedCountText = layout.findViewById(R.id.data_saved_count_text);
        dataSavedText = layout.findViewById(R.id.data_saved_text);
        timeSavedCountText = layout.findViewById(R.id.time_saved_count_text);
        timeSavedText = layout.findViewById(R.id.time_saved_text);
        websitesLayout = layout.findViewById(R.id.wesites_layout);
        trackersLayout = layout.findViewById(R.id.trackers_layout);
        braveStatsSubSectionText = layout.findViewById(R.id.brave_stats_sub_section_text);

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
        updateBraveStatsLayoutAsync();

        dialog.setContentView(view);
        ViewParent parent = view.getParent();
        ((View)parent).getLayoutParams().height = ViewGroup.LayoutParams.MATCH_PARENT;

    }

    @Override
    public void onDestroyView() {
        getActivity().setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
        super.onDestroyView();
    }

    private void updateBraveStatsLayoutAsync() {
        new AsyncTask<Void>() {
            long adsTrackersCount;
            long totalSavedBandwidth;
            long adsTrackersCountToCheckForMonth;
            long adsTrackersCountToCheckFor3Month;
            @Override
            protected Void doInBackground() {
                adsTrackersCount =
                    mDatabaseHelper
                    .getAllStatsWithDate(BraveStatsUtil.getCalculatedDate(
                                             "yyyy-MM-dd", selectedDuration),
                                         BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", 0))
                    .size();
                totalSavedBandwidth = mDatabaseHelper.getTotalSavedBandwidthWithDate(
                                          BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", selectedDuration),
                                          BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", 0));
                adsTrackersCountToCheckForMonth =
                    mDatabaseHelper
                    .getAllStatsWithDate(
                        BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", DAYS_30),
                        BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", DAYS_7))
                    .size();
                adsTrackersCountToCheckFor3Month =
                    mDatabaseHelper
                    .getAllStatsWithDate(
                        BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", DAYS_90),
                        BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", DAYS_30))
                    .size();
                return null;
            }

            @Override
            protected void onPostExecute(Void result) {
                assert ThreadUtils.runningOnUiThread();
                if (isCancelled()) return;
                Pair<String, String> adsTrackersPair =
                    BraveStatsUtil.getBraveStatsStringFormNumberPair(adsTrackersCount, false);
                adsTrackersCountText.setText(
                    String.format(mContext.getResources().getString(R.string.ntp_stat_text),
                                  adsTrackersPair.first, adsTrackersPair.second));

                Pair<String, String> dataSavedPair =
                    BraveStatsUtil.getBraveStatsStringFormNumberPair(totalSavedBandwidth, true);
                dataSavedCountText.setText(dataSavedPair.first);
                boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(mContext);
                if (isTablet) {
                    adsTrackersText.setText(
                        String.format(mContext.getResources().getString(R.string.trackers_and_ads),
                                      dataSavedPair.second));
                    dataSavedText.setText(
                        String.format(mContext.getResources().getString(R.string.data_saved_tablet_text),
                                      dataSavedPair.second));
                } else {
                    adsTrackersText.setText(
                        String.format(mContext.getResources().getString(R.string.ads_trackers_text),
                                      dataSavedPair.second));
                    dataSavedText.setText(
                        String.format(mContext.getResources().getString(R.string.data_saved_text),
                                      dataSavedPair.second));
                }

                long timeSavedCount = adsTrackersCount * BraveStatsUtil.MILLISECONDS_PER_ITEM;
                Pair<String, String> timeSavedPair =
                        BraveStatsUtil.getBraveStatsStringFromTime(timeSavedCount / 1000);
                timeSavedCountText.setText(
                        String.format(mContext.getResources().getString(R.string.ntp_stat_text),
                                timeSavedPair.first, timeSavedPair.second));
                timeSavedText.setText(mContext.getResources().getString(R.string.time_saved_text));

                if (adsTrackersCount > 0) {
                    emptyDataLayout.setVisibility(View.GONE);
                } else {
                    emptyDataLayout.setVisibility(View.VISIBLE);
                }

                // Check for month option
                if (adsTrackersCountToCheckForMonth > 0) {
                    monthRadioButton.setEnabled(true);
                    monthRadioButton.setAlpha(1.0f);
                } else {
                    monthRadioButton.setEnabled(false);
                    monthRadioButton.setAlpha(0.2f);
                }

                // Check for 3 month option
                if (adsTrackersCountToCheckFor3Month > 0) {
                    monthsRadioButton.setEnabled(true);
                    monthsRadioButton.setAlpha(1.0f);
                } else {
                    monthsRadioButton.setEnabled(false);
                    monthsRadioButton.setAlpha(0.2f);
                }
                showWebsitesTrackers();
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    private void showWebsitesTrackers() {
        new AsyncTask<Void>() {
            List<Pair<String, Integer>> websiteTrackers;
            @Override
            protected Void doInBackground() {
                if (selectedType == WEBSITES) {
                    websiteTrackers = mDatabaseHelper.getStatsWithDate(
                                          BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", selectedDuration),
                                          BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", 0));
                } else {
                    websiteTrackers = mDatabaseHelper.getSitesWithDate(
                                          BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", selectedDuration),
                                          BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", 0));
                }
                return null;
            }

            @Override
            protected void onPostExecute(Void result) {
                assert ThreadUtils.runningOnUiThread();
                if (isCancelled()) return;
                LinearLayout rootView = null;
                if (selectedType == WEBSITES) {
                    websitesLayout.setVisibility(View.VISIBLE);
                    trackersLayout.setVisibility(View.GONE);
                    rootView = websitesLayout;
                } else {
                    websitesLayout.setVisibility(View.GONE);
                    trackersLayout.setVisibility(View.VISIBLE);
                    rootView = trackersLayout;
                }

                rootView.removeAllViews();

                if (websiteTrackers.size() > 0) {
                    for (Pair<String, Integer> statPair : websiteTrackers) {
                        LayoutInflater inflater = LayoutInflater.from(mContext);
                        ViewGroup layout =
                            (ViewGroup) inflater.inflate(R.layout.tracker_item_layout, null);

                        TextView mTrackerCountText =
                            (TextView) layout.findViewById(R.id.tracker_count_text);
                        TextView mSiteText = (TextView) layout.findViewById(R.id.site_text);

                        mTrackerCountText.setText(String.valueOf(statPair.second));
                        mSiteText.setText(statPair.first);
                        if (GlobalNightModeStateProviderHolder.getInstance().isInNightMode()) {
                            mSiteText.setTextColor(mContext.getResources().getColor(
                                    R.color.brave_stats_text_dark_color));
                            mTrackerCountText.setTextColor(mContext.getResources().getColor(
                                    R.color.brave_stats_text_dark_color));
                        } else {
                            mSiteText.setTextColor(mContext.getResources().getColor(
                                    R.color.brave_stats_text_light_color));
                            mTrackerCountText.setTextColor(mContext.getResources().getColor(
                                    R.color.brave_stats_text_light_color));
                        }

                        rootView.addView(layout);
                    }
                    noDataText.setVisibility(View.GONE);
                    braveStatsSubSectionText.setVisibility(View.VISIBLE);
                } else {
                    noDataText.setVisibility(View.VISIBLE);
                    braveStatsSubSectionText.setVisibility(View.GONE);
                }
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }
}
