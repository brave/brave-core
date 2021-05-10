/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.onboarding;

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
import androidx.appcompat.app.AppCompatActivity;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveConfig;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.customtabs.CustomTabActivity;
import org.chromium.chrome.browser.night_mode.GlobalNightModeStateProviderHolder;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.util.PackageUtils;

public class P3aOnboardingActivity extends AppCompatActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_p3a_onboarding);

        boolean isFirstInstall = PackageUtils.isFirstInstall(this);

        TextView p3aOnboardingTitle = findViewById(R.id.p3a_onboarding_title);
        p3aOnboardingTitle.setText(isFirstInstall
                        ? getResources().getString(R.string.p3a_onboarding_title_text_1)
                        : getResources().getString(R.string.p3a_onboarding_title_text_2));
        CheckBox p3aOnboardingCheckbox = findViewById(R.id.p3a_onboarding_checkbox);
        boolean isP3aEnabled = true;
        try {
            // This is a hack for an unsolved JNI problem on android browsertests
            // It cannot be caught by Java because it is a native crash
            // Probably have something to do with library haven't finished loading at the time
            // this activity is initializing
            if (BraveConfig.P3A_ANDROID_BROWSERTEST)
                isP3aEnabled = false;
            else
                isP3aEnabled = BravePrefServiceBridge.getInstance().getP3AEnabled();
        } catch (Exception e) {
            Log.e("P3aOnboarding", e.getMessage());
        }
        p3aOnboardingCheckbox.setChecked(isP3aEnabled);
        p3aOnboardingCheckbox.setOnCheckedChangeListener(
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
        ImageView p3aOnboardingImg = findViewById(R.id.p3a_onboarding_img);
        // Same as above
        if (BraveConfig.P3A_ANDROID_BROWSERTEST)
            p3aOnboardingImg.setImageResource(R.drawable.ic_brave_logo);
        else
            p3aOnboardingImg.setImageResource(isFirstInstall
                            ? R.drawable.ic_brave_logo
                            : (GlobalNightModeStateProviderHolder.getInstance().isInNightMode()
                                            ? R.drawable.ic_spot_graphic_dark
                                            : R.drawable.ic_spot_graphic));
        TextView p3aOnboardingText = findViewById(R.id.p3a_onboarding_text);
        Button btnContinue = findViewById(R.id.btn_continue);
        btnContinue.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (PackageUtils.isFirstInstall(P3aOnboardingActivity.this)
                        && !OnboardingPrefManager.getInstance().isNewOnboardingShown()
                        && BraveActivity.getBraveActivity() != null) {
                    BraveActivity.getBraveActivity().showOnboardingV2(false);
                }
                OnboardingPrefManager.getInstance().setP3aOnboardingShown(true);
                OnboardingPrefManager.getInstance().setShowDefaultBrowserModalAfterP3A(true);
                finish();
            }
        });

        String productAnalysisString =
                String.format(getResources().getString(R.string.p3a_onboarding_checkbox_text,
                        getResources().getString(R.string.private_product_analysis_text)));
        int productAnalysisIndex = productAnalysisString.indexOf(
                getResources().getString(R.string.private_product_analysis_text));
        Spanned productAnalysisSpanned =
                BraveRewardsHelper.spannedFromHtmlString(productAnalysisString);
        SpannableString productAnalysisTextSS =
                new SpannableString(productAnalysisSpanned.toString());

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
                productAnalysisIndex
                        + getResources().getString(R.string.private_product_analysis_text).length(),
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        productAnalysisTextSS.setSpan(
                new ForegroundColorSpan(getResources().getColor(R.color.brave_blue_tint_color)),
                productAnalysisIndex,
                productAnalysisIndex
                        + getResources().getString(R.string.private_product_analysis_text).length(),
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        p3aOnboardingText.setMovementMethod(LinkMovementMethod.getInstance());
        p3aOnboardingText.setText(productAnalysisTextSS);
    }

    @Override
    public void onBackPressed() {}
}
