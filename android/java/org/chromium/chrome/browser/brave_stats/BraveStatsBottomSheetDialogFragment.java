/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_stats;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Dialog;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;

import com.google.android.material.bottomsheet.BottomSheetDialogFragment;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.task.AsyncTask;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.local_database.DatabaseHelper;
import org.chromium.chrome.browser.night_mode.GlobalNightModeStateProviderHolder;
import org.chromium.chrome.browser.notifications.BravePermissionUtils;
import org.chromium.chrome.browser.util.BraveTouchUtils;
import org.chromium.ui.base.DeviceFormFactor;

import java.util.List;

public class BraveStatsBottomSheetDialogFragment extends BottomSheetDialogFragment {
    public static final String TAG_FRAGMENT = "BRAVESTATS_FRAG";
    private final DatabaseHelper mDatabaseHelper = DatabaseHelper.getInstance();

    private static final int WEBSITES = 0;
    private static final int TRACKERS = 1;

    private static final int DAYS_7 = -7;
    private static final int DAYS_30 = -30;
    private static final int DAYS_90 = -90;

    private TextView mAdsTrackersCountText;
    private TextView mAdsTrackersText;
    private TextView mDataSavedCountText;
    private TextView mDataSavedText;
    private TextView mTimeSavedCountText;
    private TextView mTimeSavedText;
    private TextView mNoDataText;
    private TextView mBraveStatsSubSectionText;
    private LinearLayout mEmptyDataLayout;
    private LinearLayout mWebsitesLayout;
    private LinearLayout mTrackersLayout;

    private RadioButton mWeekRadioButton;
    private RadioButton mMonthRadioButton;
    private RadioButton mMonthsRadioButton;
    private RadioButton mTrackersRadioButton;
    private RadioButton mWebsitesRadioButton;
    private View mStatsNotificationView;

    private int mSelectedType = WEBSITES;
    private int mSelectedDuration = DAYS_7;

    private Context mContext;

    public static BraveStatsBottomSheetDialogFragment newInstance() {
        return new BraveStatsBottomSheetDialogFragment();
    }

    @Override
    @SuppressLint("SourceLockedOrientationActivity")
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setStyle(STYLE_NORMAL, R.style.AppBottomSheetDialogTheme);
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

        final View view =
                LayoutInflater.from(getContext()).inflate(R.layout.brave_stats_bottom_sheet, null);

        mEmptyDataLayout = view.findViewById(R.id.brave_stats_empty_layout);

        RadioGroup durationRadioGroup = view.findViewById(R.id.duration_radio_group);
        durationRadioGroup.setOnCheckedChangeListener(
                new RadioGroup.OnCheckedChangeListener() {
                    @Override
                    public void onCheckedChanged(RadioGroup radioGroup, int checkedId) {
                        if (checkedId == R.id.week_radio) {
                            mSelectedDuration = DAYS_7;
                        } else if (checkedId == R.id.month_radio) {
                            mSelectedDuration = DAYS_30;
                        } else if (checkedId == R.id.months_radio) {
                            mSelectedDuration = DAYS_90;
                        }
                        updateBraveStatsLayoutAsync();
                    }
                });

        mWeekRadioButton = view.findViewById(R.id.week_radio);
        mMonthRadioButton = view.findViewById(R.id.month_radio);
        mMonthsRadioButton = view.findViewById(R.id.months_radio);
        mTrackersRadioButton = view.findViewById(R.id.trackers_radio);
        mWebsitesRadioButton = view.findViewById(R.id.websites_radio);

        LinearLayout layout = view.findViewById(R.id.brave_stats_layout);
        mAdsTrackersCountText = layout.findViewById(R.id.ads_trackers_count_text);
        mAdsTrackersText = layout.findViewById(R.id.ads_trackers_text);
        mDataSavedCountText = layout.findViewById(R.id.data_saved_count_text);
        mDataSavedText = layout.findViewById(R.id.data_saved_text);
        mTimeSavedCountText = layout.findViewById(R.id.time_saved_count_text);
        mTimeSavedText = layout.findViewById(R.id.time_saved_text);
        mWebsitesLayout = layout.findViewById(R.id.wesites_layout);
        mTrackersLayout = layout.findViewById(R.id.trackers_layout);
        mBraveStatsSubSectionText = layout.findViewById(R.id.brave_stats_sub_section_text);
        mStatsNotificationView = view.findViewById(R.id.brave_stats_notification_permission);

        BraveTouchUtils.ensureMinTouchTarget(mWeekRadioButton);
        BraveTouchUtils.ensureMinTouchTarget(mMonthRadioButton);
        BraveTouchUtils.ensureMinTouchTarget(mMonthsRadioButton);
        BraveTouchUtils.ensureMinTouchTarget(mTrackersRadioButton);
        BraveTouchUtils.ensureMinTouchTarget(mWebsitesRadioButton);

        RadioGroup statTypeRadioGroup = layout.findViewById(R.id.stat_type_radio_group);
        statTypeRadioGroup.setOnCheckedChangeListener(
                new RadioGroup.OnCheckedChangeListener() {
                    @Override
                    public void onCheckedChanged(RadioGroup radioGroup, int checkedId) {
                        if (checkedId == R.id.websites_radio) {
                            mSelectedType = WEBSITES;
                        } else if (checkedId == R.id.trackers_radio) {
                            mSelectedType = TRACKERS;
                        }
                        showWebsitesTrackers();
                    }
                });

        mNoDataText = layout.findViewById(R.id.empty_data_text);
        ImageView btnClose = view.findViewById(R.id.brave_stats_bottom_sheet_close);
        btnClose.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        dismiss();
                    }
                });
        updateBraveStatsLayoutAsync();
        updateNotificationView(view);

        dialog.setContentView(view);
        ViewParent parent = view.getParent();
        ((View)parent).getLayoutParams().height = ViewGroup.LayoutParams.MATCH_PARENT;

    }

    public void onRequestPermissionsResult(
            int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
            mStatsNotificationView.setVisibility(View.GONE);
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        if (!BravePermissionUtils.hasPermission(
                        getContext(), Manifest.permission.POST_NOTIFICATIONS)
                || BravePermissionUtils.isGeneralNotificationPermissionBlocked(getActivity())) {
            mStatsNotificationView.setVisibility(View.VISIBLE);
        } else {
            mStatsNotificationView.setVisibility(View.GONE);
        }
    }

    private void updateNotificationView(View view) {
        ImageView btnDismiss = view.findViewById(R.id.button_dismiss);
        btnDismiss.setOnClickListener(
                v -> {
                    mStatsNotificationView.setVisibility(View.GONE);
                });
        View notificationOnButton = view.findViewById(R.id.notification_on_button);
        notificationOnButton.setOnClickListener(
                v -> {
                    int targetSdkVersion =
                            ContextUtils.getApplicationContext()
                                    .getApplicationInfo()
                                    .targetSdkVersion;
                    if (BravePermissionUtils.isGeneralNotificationPermissionBlocked(getActivity())
                            || getActivity()
                                    .shouldShowRequestPermissionRationale(
                                            Manifest.permission.POST_NOTIFICATIONS)
                            || (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU
                                    || targetSdkVersion < Build.VERSION_CODES.TIRAMISU)) {
                        // other than android 13 redirect to
                        // setting page and for android 13 Last time don't allow selected in
                        // permission
                        // dialog, then enable through setting
                        BravePermissionUtils.notificationSettingPage(getContext());
                    } else {
                        // 1st time request permission
                        ActivityCompat.requestPermissions(
                                getActivity(),
                                new String[] {Manifest.permission.POST_NOTIFICATIONS},
                                1);
                    }
                });
    }

    @Override
    public void onDestroyView() {
        getActivity().setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
        super.onDestroyView();
    }

    private void updateBraveStatsLayoutAsync() {
        new AsyncTask<Void>() {
            long mAdsTrackersCount;
            long mTotalSavedBandwidth;
            long mAdsTrackersCountToCheckForMonth;
            long mAdsTrackersCountToCheckFor3Month;

            @Override
            protected Void doInBackground() {
                mAdsTrackersCount =
                        mDatabaseHelper
                                .getAllStatsWithDate(
                                        BraveStatsUtil.getCalculatedDate(
                                                "yyyy-MM-dd", mSelectedDuration),
                                        BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", 0))
                                .size();
                mTotalSavedBandwidth =
                        mDatabaseHelper.getTotalSavedBandwidthWithDate(
                                BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", mSelectedDuration),
                                BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", 0));
                mAdsTrackersCountToCheckForMonth =
                        mDatabaseHelper
                                .getAllStatsWithDate(
                                        BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", DAYS_30),
                                        BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", DAYS_7))
                                .size();
                mAdsTrackersCountToCheckFor3Month =
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
                        BraveStatsUtil.getBraveStatsStringFormNumberPair(mAdsTrackersCount, false);
                mAdsTrackersCountText.setText(
                        String.format(
                                mContext.getResources().getString(R.string.ntp_stat_text),
                                adsTrackersPair.first,
                                adsTrackersPair.second));

                Pair<String, String> dataSavedPair =
                        BraveStatsUtil.getBraveStatsStringFormNumberPair(
                                mTotalSavedBandwidth, true);
                mDataSavedCountText.setText(dataSavedPair.first);
                boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(mContext);
                if (isTablet) {
                    mAdsTrackersText.setText(
                            String.format(
                                    mContext.getResources().getString(R.string.trackers_and_ads),
                                    dataSavedPair.second));
                    mDataSavedText.setText(
                            String.format(
                                    mContext.getResources()
                                            .getString(R.string.data_saved_tablet_text),
                                    dataSavedPair.second));
                } else {
                    mAdsTrackersText.setText(
                            String.format(
                                    mContext.getResources().getString(R.string.ads_trackers_text),
                                    dataSavedPair.second));
                    mDataSavedText.setText(
                            String.format(
                                    mContext.getResources().getString(R.string.data_saved_text),
                                    dataSavedPair.second));
                }

                long timeSavedCount = mAdsTrackersCount * BraveStatsUtil.MILLISECONDS_PER_ITEM;
                Pair<String, String> timeSavedPair =
                        BraveStatsUtil.getBraveStatsStringFromTime(timeSavedCount / 1000);
                mTimeSavedCountText.setText(
                        String.format(
                                mContext.getResources().getString(R.string.ntp_stat_text),
                                timeSavedPair.first,
                                timeSavedPair.second));
                mTimeSavedText.setText(mContext.getResources().getString(R.string.time_saved_text));

                if (mAdsTrackersCount > 0) {
                    mEmptyDataLayout.setVisibility(View.GONE);
                } else {
                    mEmptyDataLayout.setVisibility(View.VISIBLE);
                }

                // Check for month option
                if (mAdsTrackersCountToCheckForMonth > 0) {
                    mMonthRadioButton.setEnabled(true);
                    mMonthRadioButton.setAlpha(1.0f);
                } else {
                    mMonthRadioButton.setEnabled(false);
                    mMonthRadioButton.setAlpha(0.2f);
                }

                // Check for 3 month option
                if (mAdsTrackersCountToCheckFor3Month > 0) {
                    mMonthsRadioButton.setEnabled(true);
                    mMonthsRadioButton.setAlpha(1.0f);
                } else {
                    mMonthsRadioButton.setEnabled(false);
                    mMonthsRadioButton.setAlpha(0.2f);
                }
                showWebsitesTrackers();
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    private void showWebsitesTrackers() {
        new AsyncTask<Void>() {
            List<Pair<String, Integer>> mWebsiteTrackers;

            @Override
            protected Void doInBackground() {
                if (mSelectedType == WEBSITES) {
                    mWebsiteTrackers =
                            mDatabaseHelper.getStatsWithDate(
                                    BraveStatsUtil.getCalculatedDate(
                                            "yyyy-MM-dd", mSelectedDuration),
                                    BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", 0));
                } else {
                    mWebsiteTrackers =
                            mDatabaseHelper.getSitesWithDate(
                                    BraveStatsUtil.getCalculatedDate(
                                            "yyyy-MM-dd", mSelectedDuration),
                                    BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", 0));
                }
                return null;
            }

            @Override
            protected void onPostExecute(Void result) {
                assert ThreadUtils.runningOnUiThread();
                if (isCancelled()) return;
                LinearLayout rootView = null;
                if (mSelectedType == WEBSITES) {
                    mWebsitesLayout.setVisibility(View.VISIBLE);
                    mTrackersLayout.setVisibility(View.GONE);
                    rootView = mWebsitesLayout;
                } else {
                    mWebsitesLayout.setVisibility(View.GONE);
                    mTrackersLayout.setVisibility(View.VISIBLE);
                    rootView = mTrackersLayout;
                }

                rootView.removeAllViews();

                if (mWebsiteTrackers.size() > 0) {
                    for (Pair<String, Integer> statPair : mWebsiteTrackers) {
                        LayoutInflater inflater = LayoutInflater.from(mContext);
                        ViewGroup layout =
                                (ViewGroup) inflater.inflate(R.layout.tracker_item_layout, null);

                        TextView mTrackerCountText =
                                (TextView) layout.findViewById(R.id.tracker_count_text);
                        TextView mSiteText = (TextView) layout.findViewById(R.id.site_text);

                        mTrackerCountText.setText(String.valueOf(statPair.second));
                        mSiteText.setText(statPair.first);
                        if (GlobalNightModeStateProviderHolder.getInstance().isInNightMode()) {
                            mSiteText.setTextColor(
                                    mContext.getColor(R.color.brave_stats_text_dark_color));
                            mTrackerCountText.setTextColor(
                                    mContext.getColor(R.color.brave_stats_text_dark_color));
                        } else {
                            mSiteText.setTextColor(
                                    mContext.getColor(R.color.brave_stats_text_light_color));
                            mTrackerCountText.setTextColor(
                                    mContext.getColor(R.color.brave_stats_text_light_color));
                        }

                        rootView.addView(layout);
                    }
                    mNoDataText.setVisibility(View.GONE);
                    mBraveStatsSubSectionText.setVisibility(View.VISIBLE);
                } else {
                    mNoDataText.setVisibility(View.VISIBLE);
                    mBraveStatsSubSectionText.setVisibility(View.GONE);
                }
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }
}
