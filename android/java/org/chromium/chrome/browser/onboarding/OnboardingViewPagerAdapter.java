/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.onboarding;

import android.app.Fragment;
import android.app.FragmentManager;
import android.content.Context;
import android.support.v13.app.FragmentPagerAdapter;

import org.chromium.chrome.browser.onboarding.OnViewPagerAction;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;

public class OnboardingViewPagerAdapter extends FragmentPagerAdapter {
    private final OnViewPagerAction onViewPagerAction;
    private final int onboardingType;
    private final boolean fromSettings;

    private final Context context;

    private final boolean isAdsAvailable;

    private static final int ONBOARDING_WITH_5_PAGES = 5;
    private static final int ONBOARDING_WITH_3_PAGES = 3;

    public OnboardingViewPagerAdapter(Context context, FragmentManager fm,
            OnViewPagerAction onViewPagerAction, int onboardingType, boolean fromSettings) {
        super(fm);
        this.context = context;
        this.onViewPagerAction = onViewPagerAction;
        this.onboardingType = onboardingType;
        this.fromSettings = fromSettings;
        isAdsAvailable = OnboardingPrefManager.getInstance().isAdsAvailable();
    }

    @Override
    public Fragment getItem(int position) {
        switch (onboardingType) {
            case OnboardingPrefManager.NEW_USER_ONBOARDING:
                return newUserOnboarding(position);
            case OnboardingPrefManager.EXISTING_USER_REWARDS_OFF_ONBOARDING:
                return existingUserRewardsOffOnboarding(position);
            case OnboardingPrefManager.EXISTING_USER_REWARDS_ON_ONBOARDING:
                return existingUserRewardsOnOnboarding(position);
            default:
                return null;
        }
    }

    @Override
    public int getCount() {
        if (onboardingType == OnboardingPrefManager.NEW_USER_ONBOARDING && isAdsAvailable) {
            return ONBOARDING_WITH_5_PAGES;
        } else {
            return ONBOARDING_WITH_3_PAGES;
        }
    }

    private Fragment newUserOnboarding(int position) {
        switch (position) {
            case 0:
                SearchEngineOnboardingFragment searchEngineOnboardingFragment =
                        new SearchEngineOnboardingFragment();
                searchEngineOnboardingFragment.setFromSettings(fromSettings);
                searchEngineOnboardingFragment.setOnViewPagerAction(onViewPagerAction);
                return searchEngineOnboardingFragment;
            case 1:
                BraveShieldsOnboardingFragment braveShieldsOnboardingFragment =
                        new BraveShieldsOnboardingFragment();
                braveShieldsOnboardingFragment.setOnViewPagerAction(onViewPagerAction);
                return braveShieldsOnboardingFragment;
            case 2:
                BraveRewardsOnboardingFragment braveRewardsOnboardingFragment =
                        new BraveRewardsOnboardingFragment();
                braveRewardsOnboardingFragment.setFromSettings(fromSettings);
                braveRewardsOnboardingFragment.setOnViewPagerAction(onViewPagerAction);
                return braveRewardsOnboardingFragment;
            case 3:
                BraveAdsOnboardingFragment braveAdsOnboardingFragment =
                        new BraveAdsOnboardingFragment();
                braveAdsOnboardingFragment.setFromSettings(fromSettings);
                braveAdsOnboardingFragment.setOnViewPagerAction(onViewPagerAction);
                return braveAdsOnboardingFragment;
            case 4:
                TroubleshootingOnboardingFragment troubleshootingOnboardingFragment =
                        new TroubleshootingOnboardingFragment();
                troubleshootingOnboardingFragment.setOnViewPagerAction(onViewPagerAction);
                return troubleshootingOnboardingFragment;
            default:
                return null;
        }
    }

    private Fragment newUserOnboardingNoAds(int position) {
        switch (position) {
            case 0:
                SearchEngineOnboardingFragment searchEngineOnboardingFragment =
                        new SearchEngineOnboardingFragment();
                searchEngineOnboardingFragment.setFromSettings(fromSettings);
                searchEngineOnboardingFragment.setOnViewPagerAction(onViewPagerAction);
                return searchEngineOnboardingFragment;
            case 1:
                BraveShieldsOnboardingFragment braveShieldsOnboardingFragment =
                        new BraveShieldsOnboardingFragment();
                braveShieldsOnboardingFragment.setOnViewPagerAction(onViewPagerAction);
                return braveShieldsOnboardingFragment;
            case 2:
                BraveRewardsOnboardingFragment braveRewardsOnboardingFragment =
                        new BraveRewardsOnboardingFragment();
                braveRewardsOnboardingFragment.setFromSettings(fromSettings);
                braveRewardsOnboardingFragment.setOnViewPagerAction(onViewPagerAction);
                return braveRewardsOnboardingFragment;
            default:
                return null;
        }
    }

    private Fragment existingUserRewardsOffOnboarding(int position) {
        switch (position) {
            case 0:
                BraveRewardsOnboardingFragment braveRewardsOnboardingFragment =
                        new BraveRewardsOnboardingFragment();
                braveRewardsOnboardingFragment.setOnViewPagerAction(onViewPagerAction);
                braveRewardsOnboardingFragment.setOnboardingType(onboardingType);
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

    private Fragment existingUserRewardsOnOnboarding(int position) {
        switch (position) {
            case 0:
                BraveRewardsOnboardingFragment braveRewardsOnboardingFragment =
                        new BraveRewardsOnboardingFragment();
                braveRewardsOnboardingFragment.setOnViewPagerAction(onViewPagerAction);
                braveRewardsOnboardingFragment.setOnboardingType(onboardingType);
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
}
