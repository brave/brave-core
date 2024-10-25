/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.onboarding;

import android.os.Bundle;
import android.os.CountDownTimer;
import android.os.Handler;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.accessibility.AccessibilityEvent;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import androidx.fragment.app.Fragment;

import com.airbnb.lottie.LottieAnimationView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.firstrun.FirstRunFragment;
import org.chromium.chrome.browser.notifications.BraveOnboardingNotification;

public class BraveAdsOnboardingFragment extends Fragment implements FirstRunFragment {
    private OnViewPagerAction onViewPagerAction;

    private int progress;
    private int endTime = 3;
    private boolean mNativeInitialized;

    private LottieAnimationView animatedView;

    private TextView tvTitle;
    private TextView tvTimer;

    private ProgressBar progressBarView;

    private CountDownTimer countDownTimer;
    private LinearLayout countDownLayout;
    private LinearLayout actionLayout;

    private Button btnStartBrowsing;
    private Button btnDidntSeeAd;

    public BraveAdsOnboardingFragment() {
        // Required empty public constructor
    }

    @Override
    public View onCreateView(
        LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        // Inflate the layout for this fragment
        View root = inflater.inflate(R.layout.fragment_brave_ads_onboarding, container, false);

        initializeViews(root);

        setActions();

        return root;
    }

    @Override
    public void setUserVisibleHint(boolean isVisibleToUser) {
        super.setUserVisibleHint(isVisibleToUser);
        if (isVisibleToUser) {
            countDownLayout.setVisibility(View.VISIBLE);
            tvTitle.setVisibility(View.VISIBLE);
            actionLayout.setVisibility(View.GONE);
            btnDidntSeeAd.setVisibility(View.GONE);
            OnboardingPrefManager.isNotification = true;
            if (animatedView != null) {
                animatedView.playAnimation();
            }
        }
    }

    @Override
    public void onNativeInitialized() {
        assert !mNativeInitialized;

        mNativeInitialized = true;
        startCountdown();
    }

    @Override
    public void setInitialA11yFocus() {
        if (tvTitle == null) return;
        tvTitle.sendAccessibilityEvent(AccessibilityEvent.TYPE_VIEW_FOCUSED);
    }

    private void setActions() {
        btnStartBrowsing.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                // assert onViewPagerAction != null;
                // if (onViewPagerAction != null) onViewPagerAction.onStartBrowsing();
            }
        });

        btnDidntSeeAd.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                // assert onViewPagerAction != null;
                // if (onViewPagerAction != null) onViewPagerAction.onDidntSeeAd();
            }
        });
    }

    private void initializeViews(View root) {
        animatedView = root.findViewById(R.id.bg_image);

        tvTitle = root.findViewById(R.id.section_title);

        countDownLayout = root.findViewById(R.id.count_down_layout);
        actionLayout = root.findViewById(R.id.action_layout);

        tvTimer = root.findViewById(R.id.tv_timer);

        btnStartBrowsing = root.findViewById(R.id.btn_start_browsing);
        btnDidntSeeAd = root.findViewById(R.id.btn_didnt_see_ad);

        progressBarView = root.findViewById(R.id.view_progress_bar);
    }

    public void setOnViewPagerAction(OnViewPagerAction onViewPagerAction) {
        this.onViewPagerAction = onViewPagerAction;
    }

    private void startCountdown() {
        BraveOnboardingNotification.cancelOnboardingNotification(getActivity());

        if (countDownTimer != null) countDownTimer.cancel();

        progress = 0;

        countDownTimer =
                new CountDownTimer((long) endTime * 1000, 100) {
                    @Override
                    public void onTick(long millisUntilFinished) {
                        setProgress(progress, endTime);
                        progress = progress + 100;
                        tvTimer.setText(String.valueOf((millisUntilFinished / 1000) + 1));
                    }

                    @Override
                    public void onFinish() {
                        setProgress(progress, endTime);
                        tvTimer.setText("0");

                        OnboardingPrefManager.getInstance().onboardingNotification();
                        new Handler()
                                .postDelayed(
                                        new Runnable() {
                                            @Override
                                            public void run() {
                                                assert onViewPagerAction != null;
                                                if (onViewPagerAction != null)
                                                    onViewPagerAction.onNext();
                                            }
                                        },
                                        1000);
                    }
                };
        countDownTimer.start();
    }

    private void setProgress(int startTime, int endTime) {
        progressBarView.setMax(endTime * 1000);
        progressBarView.setSecondaryProgress(endTime * 1000);
        progressBarView.setProgress(startTime);
    }
}
