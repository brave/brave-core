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
    private OnViewPagerAction mOnViewPagerAction;

    private int mProgress;
    private static final int END_TIME_SECONDS = 3;
    private boolean mNativeInitialized;

    private LottieAnimationView mAnimatedView;

    private TextView mTvTitle;
    private TextView mTvTimer;

    private ProgressBar mProgressBarView;

    private CountDownTimer mCountDownTimer;
    private LinearLayout mCountDownLayout;
    private LinearLayout mActionLayout;

    private Button mBtnStartBrowsing;
    private Button mBtnDidntSeeAd;

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
            mCountDownLayout.setVisibility(View.VISIBLE);
            mTvTitle.setVisibility(View.VISIBLE);
            mActionLayout.setVisibility(View.GONE);
            mBtnDidntSeeAd.setVisibility(View.GONE);
            OnboardingPrefManager.isNotification = true;
            if (mAnimatedView != null) {
                mAnimatedView.playAnimation();
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
        if (mTvTitle == null) return;
        mTvTitle.sendAccessibilityEvent(AccessibilityEvent.TYPE_VIEW_FOCUSED);
    }

    private void setActions() {
        mBtnStartBrowsing.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        // assert onViewPagerAction != null;
                        // if (onViewPagerAction != null) onViewPagerAction.onStartBrowsing();
                    }
                });

        mBtnDidntSeeAd.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        // assert onViewPagerAction != null;
                        // if (onViewPagerAction != null) onViewPagerAction.onDidntSeeAd();
                    }
                });
    }

    private void initializeViews(View root) {
        mAnimatedView = root.findViewById(R.id.bg_image);

        mTvTitle = root.findViewById(R.id.section_title);

        mCountDownLayout = root.findViewById(R.id.count_down_layout);
        mActionLayout = root.findViewById(R.id.action_layout);

        mTvTimer = root.findViewById(R.id.tv_timer);

        mBtnStartBrowsing = root.findViewById(R.id.btn_start_browsing);
        mBtnDidntSeeAd = root.findViewById(R.id.btn_didnt_see_ad);

        mProgressBarView = root.findViewById(R.id.view_progress_bar);
    }

    public void setOnViewPagerAction(OnViewPagerAction onViewPagerAction) {
        mOnViewPagerAction = onViewPagerAction;
    }

    private void startCountdown() {
        BraveOnboardingNotification.cancelOnboardingNotification();

        if (mCountDownTimer != null) mCountDownTimer.cancel();

        mProgress = 0;

        mCountDownTimer =
                new CountDownTimer((long) END_TIME_SECONDS * 1000, 100) {
                    @Override
                    public void onTick(long millisUntilFinished) {
                        setProgress(mProgress, END_TIME_SECONDS);
                        mProgress = mProgress + 100;
                        mTvTimer.setText(String.valueOf((millisUntilFinished / 1000) + 1));
                    }

                    @Override
                    public void onFinish() {
                        setProgress(mProgress, END_TIME_SECONDS);
                        mTvTimer.setText("0");

                        OnboardingPrefManager.getInstance().onboardingNotification();
                        new Handler()
                                .postDelayed(
                                        new Runnable() {
                                            @Override
                                            public void run() {
                                                assert mOnViewPagerAction != null;
                                                if (mOnViewPagerAction != null) {
                                                    mOnViewPagerAction.onNext();
                                                }
                                            }
                                        },
                                        1000);
                    }
                };
        mCountDownTimer.start();
    }

    private void setProgress(int startTime, int endTime) {
        mProgressBarView.setMax(endTime * 1000);
        mProgressBarView.setSecondaryProgress(endTime * 1000);
        mProgressBarView.setProgress(startTime);
    }
}
