/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.onboarding;

import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v4.view.ViewPager;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.view.animation.Animation;
import android.view.animation.TranslateAnimation;
import android.widget.Toast;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.onboarding.NonSwipeableViewPager;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;
import org.chromium.chrome.browser.onboarding.OnboardingViewPagerAdapter;

import java.util.Calendar;
import java.util.Date;

public class OnboardingActivity extends AppCompatActivity implements OnViewPagerAction {
    private NonSwipeableViewPager viewPager;
    private int onboardingType;
    private boolean fromSettings;

    private static final int DAYS_60 = 60;
    private static final int DAYS_120 = 120;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_onboarding);

        Intent intent = getIntent();
        if (intent != null) {
            onboardingType = intent.getIntExtra(OnboardingPrefManager.ONBOARDING_TYPE,
                    OnboardingPrefManager.NEW_USER_ONBOARDING);
            fromSettings = intent.getBooleanExtra(OnboardingPrefManager.FROM_SETTINGS, false);
        }

        OnboardingViewPagerAdapter onboardingViewPagerAdapter = new OnboardingViewPagerAdapter(
                this, getFragmentManager(), this, onboardingType, fromSettings);
        viewPager = findViewById(R.id.view_pager);
        viewPager.setAdapter(onboardingViewPagerAdapter);
    }

    @Override
    public void onSkip() {
        if (!fromSettings) {
            Calendar calender = Calendar.getInstance();
            calender.setTime(new Date());
            calender.add(Calendar.DATE,
                    OnboardingPrefManager.getInstance().getPrefOnboardingSkipCount() == 0
                            ? DAYS_60
                            : DAYS_120);

            OnboardingPrefManager.getInstance().setPrefNextOnboardingDate(
                    calender.getTimeInMillis());
            OnboardingPrefManager.getInstance().setPrefOnboardingSkipCount();
        }
        finish();
    }

    @Override
    public void onNext() {
        viewPager.setCurrentItem(viewPager.getCurrentItem() + 1);
    }

    @Override
    public void onStartBrowsing() {
        if (!fromSettings) {
            OnboardingPrefManager.getInstance().setPrefOnboardingEnabled(false);
        }
        Intent intent = new Intent(this, ChromeTabbedActivity.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT);
        startActivity(intent);
    }

    @Override
    public void onDidntSeeAd() {
        viewPager.setCurrentItem(viewPager.getCurrentItem() + 1);
    }

    @Override
    public void onBackPressed() {
        if (fromSettings) {
            if (OnboardingPrefManager.isNotification && viewPager.getCurrentItem() == 3) {
            } else if (viewPager.getCurrentItem() > 0) {
                viewPager.setCurrentItem(viewPager.getCurrentItem() - 1);
            } else {
                super.onBackPressed();
            }
        }
    }
}
