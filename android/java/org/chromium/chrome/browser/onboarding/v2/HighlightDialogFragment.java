/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.onboarding.v2;

import android.annotation.SuppressLint;
import android.app.Dialog;
import android.content.pm.ActivityInfo;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.ImageView;

import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.FragmentManager;
import androidx.viewpager.widget.ViewPager;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.brave_stats.BraveStatsUtil;
import org.chromium.chrome.browser.notifications.retention.RetentionNotificationUtil;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;

import java.util.Arrays;
import java.util.List;
import java.util.Locale;

public class HighlightDialogFragment extends DialogFragment {
    final public static String TAG_FRAGMENT = "HIGHLIGHT_FRAG";
    private final static String NTP_TUTORIAL_PAGE = "https://brave.com/ja/android-ntp-tutorial";

    public interface HighlightDialogListener {
        void onNextPage();
        void onLearnMore();
    }

    private static final List<Integer> highlightViews =
            Arrays.asList(
                    R.id.brave_stats_ads,
                    R.id.brave_stats_data_saved,
                    R.id.brave_stats_time,
                    R.id.ntp_stats_layout);

    private HighlightView highlightView;
    private ViewPager viewpager;
    private boolean isFromStats;

    private OnboardingV2PagerAdapter onboardingV2PagerAdapter;

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        Dialog dialog = new Dialog(getActivity(), android.R.style.Theme_Black_NoTitleBar);
        dialog.getWindow().setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        dialog.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_DIM_BEHIND);
        return dialog;
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {}

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.highlight_layout, container);
        highlightView = view.findViewById(R.id.highlight_view);

        onboardingV2PagerAdapter = new OnboardingV2PagerAdapter(getChildFragmentManager());
        onboardingV2PagerAdapter.setHighlightListener(highlightDialogListener);
        onboardingV2PagerAdapter.setFromStats(isFromStats);

        // Set up the ViewPager with the sections adapter.
        viewpager = view.findViewById(R.id.viewpager);
        viewpager.setAdapter(onboardingV2PagerAdapter);
        viewpager.addOnPageChangeListener(new ViewPager.OnPageChangeListener() {
            @Override
            public void onPageScrolled(int position, float positionOffset, int positionOffsetPixels) {
                if (positionOffset == 0) {
                    highlightView(isFromStats ? 3 : position);
                }
            }

            @Override
            public void onPageSelected(int position) {

            }

            @Override
            public void onPageScrollStateChanged(int state) {

            }
        });

        ImageView btnClose = view.findViewById(R.id.onboarding_bottom_sheet_close);
        btnClose.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                checkAndOpenNtpPage();
                dismiss();
            }
        });

        return view;
    }

    @Override
    public void onDestroyView() {
        /*
         * https://code.google.com/p/android/issues/detail?id=17423
         */
        if (getDialog() != null && getRetainInstance()) {
            getDialog().setDismissMessage(null);
        }

        getActivity().setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);

        super.onDestroyView();
    }

    @Override
    @SuppressLint("SourceLockedOrientationActivity")
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Fragment locked in portrait screen orientation
        getActivity().setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);

        Bundle bundle = getArguments();
        if (bundle != null) {
            isFromStats = bundle.getBoolean(OnboardingPrefManager.FROM_STATS, false);
        }
    }

    @Override
    public void show(FragmentManager manager, String tag) {
        if (tag != null && tag.equals(TAG_FRAGMENT)) {
            // we do not show it twice
            if (manager.findFragmentByTag(tag) == null) {
                super.show(manager, tag);
            }
        } else {
            super.show(manager, tag);
        }
    }

    private void highlightView(int position) {
        int viewId;
        if (position == 3) {
            viewId = R.id.ntp_stats_layout;
        } else {
            viewId = highlightViews.get(position);
        }
        View view = getActivity().findViewById(viewId);
        if (view != null) {
            HighlightItem item = new HighlightItem(view);
            highlightView.setHighlightItem(item);
            if (position == 3) {
                highlightView.setShouldShowHighlight(false);
            } else {
                highlightView.setShouldShowHighlight(true);
            }
        }
    }

    private HighlightDialogListener highlightDialogListener = new HighlightDialogListener() {
        @Override
        public void onNextPage() {
            if (viewpager != null) {
                if (!OnboardingPrefManager.getInstance().isBraveStatsEnabled()) {
                    OnboardingPrefManager.getInstance().setBraveStatsEnabled(true);
                    RetentionNotificationUtil.scheduleNotificationForEverySunday(getActivity(), RetentionNotificationUtil.EVERY_SUNDAY);
                    if (onboardingV2PagerAdapter != null) {
                        onboardingV2PagerAdapter.notifyDataSetChanged();
                    }
                }
                int currentPage = viewpager.getCurrentItem();
                if ((OnboardingPrefManager.getInstance().isBraveStatsEnabled() && currentPage == 2)
                        || currentPage == 3
                        || isFromStats) {
                    dismiss();
                    BraveStatsUtil.showBraveStats();
                    checkAndOpenNtpPage();
                } else {
                    viewpager.setCurrentItem(currentPage + 1);
                }
            }
        }

        @Override
        public void onLearnMore() {
            dismiss();
            //Start from beginning
            ((BraveActivity)getActivity()).showOnboardingV2(false);
        }
    };

    private void checkAndOpenNtpPage() {
        String countryCode = Locale.getDefault().getCountry();
        if (((BraveActivity) getActivity()) != null && countryCode.equals("JP")) {
            ((BraveActivity) getActivity()).openNewOrSelectExistingTab(NTP_TUTORIAL_PAGE);
        }
    }
}
