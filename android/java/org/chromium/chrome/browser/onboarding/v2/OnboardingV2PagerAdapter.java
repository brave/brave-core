/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.onboarding.v2;

import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentPagerAdapter;

import org.chromium.chrome.browser.onboarding.v2.HighlightDialogFragment.HighlightDialogListener;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;

public class OnboardingV2PagerAdapter extends FragmentPagerAdapter {

    private HighlightDialogListener highlightDialogListener;
    private boolean isFromStats;

    public OnboardingV2PagerAdapter(FragmentManager fm) {
        super(fm);
    }

    @Override
    public Fragment getItem(int position) {
        OnboardingV2Fragment onboardingV2Fragment = new OnboardingV2Fragment();
        onboardingV2Fragment.setPosition(position);
        onboardingV2Fragment.setHighlightListener(highlightDialogListener);
        onboardingV2Fragment.setFromStats(isFromStats);

        return onboardingV2Fragment;
    }

    @Override
    public int getCount() {
        if (OnboardingPrefManager.getInstance().isBraveStatsEnabled()) {
            return 3;
        } else {
            return 4;
        }
    }

    public void setHighlightListener(HighlightDialogListener highlightDialogListener) {
        this.highlightDialogListener = highlightDialogListener;
    }

    public void setFromStats(boolean isFromStats) {
        this.isFromStats = isFromStats;
    }
}