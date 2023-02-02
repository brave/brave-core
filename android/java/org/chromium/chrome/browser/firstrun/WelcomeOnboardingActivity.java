/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.firstrun;

import static org.chromium.ui.base.ViewUtils.dpToPx;

import android.animation.LayoutTransition;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Animation;
import android.view.animation.Transformation;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveLocalState;
import org.chromium.chrome.browser.customtabs.CustomTabActivity;
import org.chromium.chrome.browser.metrics.UmaSessionStats;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.preferences.ChromePreferenceKeys;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.privacy.settings.PrivacyPreferencesManagerImpl;
import org.chromium.chrome.browser.set_default_browser.BraveSetDefaultBrowserUtils;
import org.chromium.chrome.browser.util.BraveConstants;
import org.chromium.chrome.browser.util.PackageUtils;
import org.chromium.ui.base.DeviceFormFactor;

import java.lang.Math;
import java.util.Locale;

public class WelcomeOnboardingActivity extends FirstRunActivityBase {
    // mInitializeViewsDone and mInvokePostWorkAtInitializeViews are accessed
    // from the same thread, so no need to use extra locks
    private static final String P3A_URL =
            "https://support.brave.com/hc/en-us/articles/9140465918093-What-is-P3A-in-Brave";
    private boolean mInitializeViewsDone;
    private boolean mInvokePostWorkAtInitializeViews;
    private boolean mIsP3aEnabled;
    private boolean mIsTablet;
    private FirstRunFlowSequencer mFirstRunFlowSequencer;
    private int mCurrentStep = -1;

    private View mVLeafAlignTop;
    private View mVLeafAlignBottom;
    private ImageView mIvBackground;
    private ImageView mIvLeafTop;
    private ImageView mIvLeafBottom;
    private ImageView mIvBrave;
    private ImageView mIvArrowDown;
    private LinearLayout mLayoutCard;
    private LinearLayout mLayoutCrash;
    private LinearLayout mLayoutP3a;
    private TextView mTvWelcome;
    private TextView mTvCard;
    private TextView mTvDefault;
    private Button mBtnPositive;
    private Button mBtnNegative;
    private CheckBox mCheckboxCrash;
    private CheckBox mCheckboxP3a;

    private void initializeViews() {
        assert !mInitializeViewsDone;
        setContentView(R.layout.activity_welcome_onboarding);

        mIsTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(this);

        initViews();
        onClickViews();

        mInitializeViewsDone = true;
        if (mInvokePostWorkAtInitializeViews) {
            finishNativeInitializationPostWork();
        }
    }

    private void initViews() {
        mIvBackground = findViewById(R.id.iv_background);
        mIvLeafTop = findViewById(R.id.iv_leaf_top);
        mIvLeafBottom = findViewById(R.id.iv_leaf_bottom);
        mVLeafAlignTop = findViewById(R.id.view_leaf_top_align);
        mVLeafAlignBottom = findViewById(R.id.view_leaf_bottom_align);
        mIvBrave = findViewById(R.id.iv_brave);
        mIvArrowDown = findViewById(R.id.iv_arrow_down);
        mLayoutCard = findViewById(R.id.layout_card);
        mLayoutCrash = findViewById(R.id.layout_crash);
        mLayoutP3a = findViewById(R.id.layout_p3a);
        mTvWelcome = findViewById(R.id.tv_welcome);
        mTvCard = findViewById(R.id.tv_card);
        mTvDefault = findViewById(R.id.tv_default);
        mCheckboxCrash = findViewById(R.id.checkbox_crash);
        mCheckboxP3a = findViewById(R.id.checkbox_p3a);
        mBtnPositive = findViewById(R.id.btn_positive);
        mBtnNegative = findViewById(R.id.btn_negative);
        LinearLayout layoutData = findViewById(R.id.layout_data);
        LayoutTransition layoutTransition = new LayoutTransition();
        layoutTransition.setDuration(1000);
        if (layoutData != null) {
            layoutData.setLayoutTransition(layoutTransition);
        }

        int margin = mIsTablet ? 200 : 50;

        if (mVLeafAlignTop != null) {
            ViewGroup.MarginLayoutParams topLeafParams =
                    (ViewGroup.MarginLayoutParams) mVLeafAlignTop.getLayoutParams();
            topLeafParams.bottomMargin = margin;
            mVLeafAlignTop.setLayoutParams(topLeafParams);
        }

        if (mVLeafAlignBottom != null) {
            ViewGroup.MarginLayoutParams bottomLeafParams =
                    (ViewGroup.MarginLayoutParams) mVLeafAlignBottom.getLayoutParams();
            bottomLeafParams.topMargin = margin;
            mVLeafAlignBottom.setLayoutParams(bottomLeafParams);
        }
    }

    private void onClickViews() {
        if (mBtnPositive != null) {
            mBtnPositive.setOnClickListener(view -> {
                if (mCurrentStep == 1
                        && !BraveSetDefaultBrowserUtils.isBraveSetAsDefaultBrowser(this)) {
                    BraveSetDefaultBrowserUtils.setDefaultBrowser(this);
                    if (!BraveSetDefaultBrowserUtils.supportsDefaultRoleManager()) {
                        nextOnboardingStep();
                    }
                } else {
                    nextOnboardingStep();
                }
            });
        }

        if (mBtnNegative != null) {
            mBtnNegative.setOnClickListener(view -> {
                if (mCurrentStep == 2) {
                    CustomTabActivity.showInfoPage(this, P3A_URL);
                } else {
                    nextOnboardingStep();
                }
            });
        }
    }

    private void startTimer(int delayMillis) {
        new Handler().postDelayed(this::nextOnboardingStep, delayMillis);
    }

    private void nextOnboardingStep() {
        mCurrentStep++;
        if (mCurrentStep == 0) {
            int margin = mIsTablet ? 100 : 0;
            setLeafAnimation(mVLeafAlignTop, mIvLeafTop, 1f, margin, true);
            setLeafAnimation(mVLeafAlignBottom, mIvLeafBottom, 1f, margin, false);
            setFadeInAnimation(mTvWelcome, 200);
            mIvBrave.animate().scaleX(0.8f).scaleY(0.8f).setDuration(1000);
            new Handler().postDelayed(new Runnable() {
                @Override
                public void run() {
                    mTvWelcome.animate()
                            .translationYBy(-dpToPx(WelcomeOnboardingActivity.this, 20))
                            .setDuration(3000)
                            .start();
                }
            }, 200);

            startTimer(3000);

        } else if (mCurrentStep == 1) {
            int margin = mIsTablet ? 200 : 30;
            setLeafAnimation(mVLeafAlignTop, mIvLeafTop, 1.3f, margin, true);
            setLeafAnimation(mVLeafAlignBottom, mIvLeafBottom, 1.3f, margin, false);

            if (BraveSetDefaultBrowserUtils.isBraveSetAsDefaultBrowser(this)) {
                mBtnPositive.setText(getResources().getString(R.string.continue_text));
                mBtnNegative.setVisibility(View.GONE);
            }
            mTvWelcome.setVisibility(View.GONE);
            mLayoutCard.setVisibility(View.VISIBLE);
            mIvArrowDown.setVisibility(View.VISIBLE);

            String countryCode = Locale.getDefault().getCountry();
            if (countryCode.equals(BraveConstants.INDIA_COUNTRY_CODE)) {
                mTvCard.setText(getResources().getString(R.string.privacy_onboarding_india));
                mTvDefault.setText(getResources().getString(R.string.onboarding_set_default_india));
            }

        } else if (mCurrentStep == 2) {
            int margin = mIsTablet ? 250 : 60;
            setLeafAnimation(mVLeafAlignTop, mIvLeafTop, 1.5f, margin, true);
            setLeafAnimation(mVLeafAlignBottom, mIvLeafBottom, 1.5f, margin, false);

            mLayoutCard.setVisibility(View.GONE);
            mTvDefault.setVisibility(View.GONE);
            mIvArrowDown.setVisibility(View.GONE);

            mTvCard.setText(getResources().getString(R.string.p3a_title));
            mBtnPositive.setText(getResources().getString(R.string.continue_text));
            mBtnNegative.setText(getResources().getString(R.string.learn_more_onboarding));
            mBtnNegative.setVisibility(View.VISIBLE);

            if (PackageUtils.isFirstInstall(this)
                    && !OnboardingPrefManager.getInstance().isP3aCrashReportingMessageShown()) {
                mCheckboxCrash.setChecked(true);
                UmaSessionStats.changeMetricsReportingConsent(true);
                OnboardingPrefManager.getInstance().setP3aCrashReportingMessageShown(true);
            } else {
                boolean isCrashReporting = false;
                try {
                    isCrashReporting = PrivacyPreferencesManagerImpl.getInstance()
                                               .isUsageAndCrashReportingPermittedByUser();

                } catch (Exception e) {
                    Log.e("isCrashReportingOnboarding", e.getMessage());
                }
                mCheckboxCrash.setChecked(isCrashReporting);
            }

            mCheckboxCrash.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                    try {
                        UmaSessionStats.changeMetricsReportingConsent(isChecked);
                    } catch (Exception e) {
                        Log.e("CrashReportingOnboarding", e.getMessage());
                    }
                }
            });

            boolean isP3aEnabled = true;

            try {
                isP3aEnabled = BraveLocalState.get().getBoolean(BravePref.P3A_ENABLED);
            } catch (Exception e) {
                Log.e("P3aOnboarding", e.getMessage());
            }

            mCheckboxP3a.setChecked(isP3aEnabled);
            mCheckboxP3a.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                    try {
                        BraveLocalState.get().setBoolean(BravePref.P3A_ENABLED, isChecked);
                        BraveLocalState.get().setBoolean(BravePref.P3A_NOTICE_ACKNOWLEDGED, true);
                        BraveLocalState.commitPendingWrite();
                    } catch (Exception e) {
                        Log.e("P3aOnboarding", e.getMessage());
                    }
                }
            });

            mTvCard.setVisibility(View.VISIBLE);
            mLayoutCrash.setVisibility(View.VISIBLE);
            mLayoutP3a.setVisibility(View.VISIBLE);
            mLayoutCard.setVisibility(View.VISIBLE);
            mIvArrowDown.setVisibility(View.VISIBLE);
        } else {
            OnboardingPrefManager.getInstance().setP3aOnboardingShown(true);
            OnboardingPrefManager.getInstance().setOnboardingSearchBoxTooltip(true);
            FirstRunStatus.setFirstRunFlowComplete(true);
            SharedPreferencesManager.getInstance().writeBoolean(
                    ChromePreferenceKeys.FIRST_RUN_CACHED_TOS_ACCEPTED, true);
            FirstRunUtils.setEulaAccepted();
            finish();
            sendFirstRunCompletePendingIntent();
        }
    }

    private void setFadeInAnimation(View view, int duration) {
        view.animate().alpha(1f).setDuration(duration).withEndAction(
                () -> view.setVisibility(View.VISIBLE));
    }

    private void setLeafAnimation(View leafAlignView, ImageView leafView, float scale,
            float leafMargin, boolean isTopLeaf) {
        if (leafMargin > 0) {
            int margin = (int) dpToPx(this, leafMargin);
            Animation animation = new Animation() {
                @Override
                protected void applyTransformation(float interpolatedTime, Transformation t) {
                    ViewGroup.MarginLayoutParams layoutParams =
                            (ViewGroup.MarginLayoutParams) leafAlignView.getLayoutParams();
                    if (isTopLeaf) {
                        layoutParams.bottomMargin = margin
                                - (int) ((margin - layoutParams.bottomMargin) * interpolatedTime);
                    } else {
                        layoutParams.topMargin = margin
                                - (int) ((margin - layoutParams.topMargin) * interpolatedTime);
                    }
                    leafAlignView.setLayoutParams(layoutParams);
                }
            };
            animation.setDuration(800);
            leafAlignView.startAnimation(animation);
        }
        if (leafView != null) {
            leafView.animate().scaleX(scale).scaleY(scale).setDuration(800);
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (resultCode == RESULT_OK
                && requestCode == BraveConstants.DEFAULT_BROWSER_ROLE_REQUEST_CODE) {
            BraveSetDefaultBrowserUtils.setBraveDefaultSuccess();
        }
        nextOnboardingStep();
        super.onActivityResult(requestCode, resultCode, data);
    }

    private void finishNativeInitializationPostWork() {
        assert mInitializeViewsDone;
        startTimer(1000);
    }

    @Override
    public void finishNativeInitialization() {
        ThreadUtils.assertOnUiThread();
        super.finishNativeInitialization();

        if (mInitializeViewsDone) {
            finishNativeInitializationPostWork();
        } else {
            mInvokePostWorkAtInitializeViews = true;
        }
    }

    @Override
    public void onBackPressed() {}

    @Override
    public void triggerLayoutInflation() {
        super.triggerLayoutInflation();

        mFirstRunFlowSequencer = new FirstRunFlowSequencer(this, getChildAccountStatusSupplier()) {
            @Override
            public void onFlowIsKnown(Bundle freProperties) {
                initializeViews();
            }
        };
        mFirstRunFlowSequencer.start();
        onInitialLayoutInflationComplete();
    }
}
