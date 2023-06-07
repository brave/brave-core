/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.onboarding;

import android.app.Activity;
import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.custom_layout.NonSwipeableViewPager;

public class OnboardingActivity extends AppCompatActivity implements OnViewPagerAction {
    private static final String TAG = "OnboardingActivity";

    private NonSwipeableViewPager viewPager;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_onboarding);

        OnboardingPrefManager.getInstance().setOnboardingShown(true);

        OnboardingViewPagerAdapter onboardingViewPagerAdapter = new OnboardingViewPagerAdapter(
            this, getSupportFragmentManager(), this);
        viewPager = findViewById(R.id.view_pager);
        viewPager.setAdapter(onboardingViewPagerAdapter);
        try {
            BraveActivity.getBraveActivity().hideRewardsOnboardingIcon();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "onCreate " + e);
        }
    }

    @Override
    public void onSkip() {
        finish();
    }

    @Override
    public void onNext() {
        viewPager.setCurrentItem(viewPager.getCurrentItem() + 1);
    }

    @Override
    public void onContinueToWallet() {
        try {
            BraveActivity.getBraveActivity().openRewardsPanel();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "onContinueToWallet " + e);
        }
        finish();
    }

    static public OnboardingActivity getOnboardingActivity() {
        for (Activity ref : ApplicationStatus.getRunningActivities()) {
            if (!(ref instanceof OnboardingActivity)) continue;

            return (OnboardingActivity)ref;
        }

        return null;
    }
}
