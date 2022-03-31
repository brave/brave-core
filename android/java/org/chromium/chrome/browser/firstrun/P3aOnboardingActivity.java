/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.firstrun;

import android.os.Bundle;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.TextPaint;
import android.text.method.LinkMovementMethod;
import android.text.style.ClickableSpan;
import android.text.style.ForegroundColorSpan;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.customtabs.CustomTabActivity;
import org.chromium.chrome.browser.night_mode.GlobalNightModeStateProviderHolder;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.preferences.ChromePreferenceKeys;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.util.PackageUtils;

import java.lang.Math;

public class P3aOnboardingActivity extends FirstRunActivityBase {
    // mInitializeViewsDone and mInvokePostWorkAtInitializeViews are accessed
    // from the same thread, so no need to use extra locks
    private boolean mInitializeViewsDone;
    private boolean mInvokePostWorkAtInitializeViews;
    private boolean mIsP3aEnabled;
    private FirstRunFlowSequencer mFirstRunFlowSequencer;
    private CheckBox mP3aOnboardingCheckbox;
    private Button mBtnContinue;

    private void initializeViews() {
        assert !mInitializeViewsDone;
        setContentView(R.layout.activity_p3a_onboarding);

        boolean isFirstInstall = PackageUtils.isFirstInstall(this);

        TextView p3aOnboardingTitle = findViewById(R.id.p3a_onboarding_title);
        p3aOnboardingTitle.setText(isFirstInstall
                        ? getResources().getString(R.string.p3a_onboarding_title_text_1)
                        : getResources().getString(R.string.p3a_onboarding_title_text_2));
        mIsP3aEnabled = true;
        mP3aOnboardingCheckbox = findViewById(R.id.p3a_onboarding_checkbox);
        mP3aOnboardingCheckbox.setChecked(mIsP3aEnabled);
        ImageView p3aOnboardingImg = findViewById(R.id.p3a_onboarding_img);
        p3aOnboardingImg.setImageResource(isFirstInstall
                        ? R.drawable.ic_brave_logo
                        : (GlobalNightModeStateProviderHolder.getInstance().isInNightMode()
                                        ? R.drawable.ic_spot_graphic_dark
                                        : R.drawable.ic_spot_graphic));
        TextView p3aOnboardingText = findViewById(R.id.p3a_onboarding_text);
        mBtnContinue = findViewById(R.id.btn_continue);
        mBtnContinue.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (PackageUtils.isFirstInstall(P3aOnboardingActivity.this)
                        && !OnboardingPrefManager.getInstance().isNewOnboardingShown()
                        && BraveActivity.getBraveActivity() != null) {
                    BraveActivity.getBraveActivity().showOnboardingV2(false);
                }
                OnboardingPrefManager.getInstance().setP3aOnboardingShown(true);
                OnboardingPrefManager.getInstance().setShowDefaultBrowserModalAfterP3A(true);
                accept();
            }
        });

        String productAnalysisString =
                String.format(getResources().getString(R.string.p3a_onboarding_checkbox_text,
                        getResources().getString(R.string.private_product_analysis_text)));
        int productAnalysisIndex = productAnalysisString.indexOf(
                getResources().getString(R.string.private_product_analysis_text));
        SpannableString productAnalysisTextSS = new SpannableString(productAnalysisString);

        ClickableSpan productAnalysisClickableSpan = new ClickableSpan() {
            @Override
            public void onClick(@NonNull View textView) {
                CustomTabActivity.showInfoPage(P3aOnboardingActivity.this, BraveActivity.P3A_URL);
            }
            @Override
            public void updateDrawState(@NonNull TextPaint ds) {
                super.updateDrawState(ds);
                ds.setUnderlineText(false);
            }
        };

        productAnalysisTextSS.setSpan(productAnalysisClickableSpan, productAnalysisIndex,
                Math.min(productAnalysisIndex
                                + getResources()
                                          .getString(R.string.private_product_analysis_text)
                                          .length(),
                        productAnalysisTextSS.length()),
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        productAnalysisTextSS.setSpan(
                new ForegroundColorSpan(getResources().getColor(R.color.brave_blue_tint_color)),
                productAnalysisIndex,
                Math.min(productAnalysisIndex
                                + getResources()
                                          .getString(R.string.private_product_analysis_text)
                                          .length(),
                        productAnalysisTextSS.length()),
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        p3aOnboardingText.setMovementMethod(LinkMovementMethod.getInstance());
        p3aOnboardingText.setText(productAnalysisTextSS);

        mInitializeViewsDone = true;
        if (mInvokePostWorkAtInitializeViews) {
            finishNativeInitializationPostWork();
        }
    }

    private void finishNativeInitializationPostWork() {
        assert mInitializeViewsDone;

        try {
            mIsP3aEnabled = BravePrefServiceBridge.getInstance().getP3AEnabled();
        } catch (Exception e) {
            Log.e("P3aOnboarding", e.getMessage());
        }
        mP3aOnboardingCheckbox.setChecked(mIsP3aEnabled);
        mP3aOnboardingCheckbox.setOnCheckedChangeListener(
                new CompoundButton.OnCheckedChangeListener() {
                    @Override
                    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                        try {
                            BravePrefServiceBridge.getInstance().setP3AEnabled(isChecked);
                            BravePrefServiceBridge.getInstance().setP3ANoticeAcknowledged(true);
                        } catch (Exception e) {
                            Log.e("P3aOnboarding", e.getMessage());
                        }
                    }
                });

        mBtnContinue.setEnabled(true);
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
    protected void triggerLayoutInflation() {
        mFirstRunFlowSequencer = new FirstRunFlowSequencer(this) {
            @Override
            public void onFlowIsKnown(Bundle freProperties) {
                initializeViews();
            }
        };
        mFirstRunFlowSequencer.start();
        onInitialLayoutInflationComplete();
    }

    public void completeOnboardingExperience() {
        FirstRunStatus.setFirstRunFlowComplete(true);
        exitOnboarding();
    }

    private void exitOnboarding() {
        finish();
        sendFirstRunCompletePendingIntent();
    }

    private void accept() {
        // Do not use existing function because it contains consent to Google crash report upload
        SharedPreferencesManager.getInstance().writeBoolean(
                ChromePreferenceKeys.FIRST_RUN_CACHED_TOS_ACCEPTED, true);
        FirstRunUtils.setEulaAccepted();
        completeOnboardingExperience();
    }
}
