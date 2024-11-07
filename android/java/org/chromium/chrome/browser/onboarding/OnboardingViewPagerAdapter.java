/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved. This Source Code Form is subject to
 * the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with
 * this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
package org.chromium.chrome.browser.onboarding;

import android.content.Context;

import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentPagerAdapter;

public class OnboardingViewPagerAdapter extends FragmentPagerAdapter {
    private final OnViewPagerAction onViewPagerAction;
    private static final int ONBOARDING_WITH_3_PAGES = 3;

    public OnboardingViewPagerAdapter(
            Context unused_context, FragmentManager fm, OnViewPagerAction onViewPagerAction) {
        super(fm);
        this.onViewPagerAction = onViewPagerAction;
    }

    @Override
    public Fragment getItem(int position) {
        switch (position) {
        case 0:
            BraveRewardsOnboardingFragment braveRewardsOnboardingFragment =
                new BraveRewardsOnboardingFragment();
            braveRewardsOnboardingFragment.setOnViewPagerAction(onViewPagerAction);
            return braveRewardsOnboardingFragment;
        case 1:
            BraveAdsOnboardingFragment braveAdsOnboardingFragment =
                new BraveAdsOnboardingFragment();
            braveAdsOnboardingFragment.setOnViewPagerAction(onViewPagerAction);
            return braveAdsOnboardingFragment;
        case 2:
            TroubleshootingOnboardingFragment troubleshootingOnboardingFragment =
                new TroubleshootingOnboardingFragment();
            troubleshootingOnboardingFragment.setOnViewPagerAction(onViewPagerAction);
            return troubleshootingOnboardingFragment;
        default:
            return null;
        }
    }

    @Override
    public int getCount() {
        return ONBOARDING_WITH_3_PAGES;
    }
}
